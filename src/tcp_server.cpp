#include "tcp_server.hpp"

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>

#include "syscall_wrapper.hpp"

using http1::TcpServer;

TcpServer::Socket::Socket(int socket_fd, TcpServer& server)
    : socket_fd_(socket_fd), server_(server) {}

void TcpServer::Socket::Write(const ByteArrayView& data,
                              const std::optional<CallBack>& callback) const {
  server_.TryWrite(socket_fd_, data, callback);
}

void TcpServer::Socket::Close() const { server_.AddToCloseQueue(socket_fd_); }

TcpServer::TcpServer(std::uint16_t port, std::size_t receive_buffer_size)
    : port_(port) {
  receive_buffer.resize(receive_buffer_size, std::byte{0});
}

TcpServer::~TcpServer() {
  close(epoll_fd_);
  close(server_fd_);
}

void TcpServer::Start() {
  server_fd_ = wrap_syscall(socket(AF_INET, SOCK_STREAM, 0),
                            "Can not create TCP socket");

  const int OPTION_ON = 1;
  wrap_syscall(setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &OPTION_ON,
                          sizeof(OPTION_ON)),
               "Can not set address reuse options");

  SetNonBlocking(server_fd_);

  sockaddr_in server_address{};
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(port_);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  wrap_syscall(bind(server_fd_, reinterpret_cast<sockaddr*>(&server_address),
                    sizeof(server_address)),
               "Can not bind server socket");
  wrap_syscall(listen(server_fd_, SOMAXCONN), "Can not start listening");

  epoll_fd_ = wrap_syscall(epoll_create(1), "Can create epoll");

  AddEvent(server_fd_, EPOLLIN | EPOLLOUT | EPOLLET);

  LoopEvents();
}

void TcpServer::LoopEvents() {
  constexpr int MAX_EPOLL_EVENTS = 64;
  std::array<epoll_event, MAX_EPOLL_EVENTS> epoll_event_list{};
  while (true) {
    const int number_of_fds = wrap_syscall(
        epoll_wait(epoll_fd_, epoll_event_list.data(), MAX_EPOLL_EVENTS, -1),
        "Error occurred while waiting for new events");
    for (int fd_iterator = 0; fd_iterator < number_of_fds; ++fd_iterator) {
      auto& current_event = epoll_event_list.at(fd_iterator);
      if (current_event.data.fd == server_fd_) {
        AcceptNewClients();
      } else {
        if ((current_event.events & EPOLLIN) != 0U) {
          ReceiveData(current_event.data.fd);
        }
        if ((current_event.events & EPOLLOUT) != 0U) {
          ContinueWrite(current_event.data.fd);
        }
      }

      if ((current_event.events & EPOLLHUP) != 0U ||
          (current_event.events & EPOLLRDHUP) != 0U) {
        AddToCloseQueue(current_event.data.fd);
      }
    }

    ConsumeCloseQueue();
  }
}

void TcpServer::SetNonBlocking(int socket_fd) {
  const int DEFAULT_FLAGS =
      wrap_syscall(fcntl(socket_fd, F_GETFL, 0), "Can not get socket flags");

  // NOLINTNEXTLINE(hicpp-signed-bitwise)
  wrap_syscall(fcntl(socket_fd, F_SETFL, DEFAULT_FLAGS | O_NONBLOCK),
               "Can not enable non-blocking for socket");
}

void TcpServer::AddEvent(int socket_fd, std::uint32_t event_flags,
                         bool update) const {
  epoll_event event{};
  event.events = event_flags;
  event.data.fd = socket_fd;
  wrap_syscall(epoll_ctl(epoll_fd_, update ? EPOLL_CTL_MOD : EPOLL_CTL_ADD,
                         socket_fd, &event),
               "Can not add/update socket event");
}

void TcpServer::AcceptNewClients() {
  while (true) {
    const int new_client_fd = accept(server_fd_, nullptr, nullptr);

    if (new_client_fd < 0) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        wrap_syscall(new_client_fd, "Can not accept new connection");
      }
      break;
    }

    SetNonBlocking(new_client_fd);
    AddEvent(new_client_fd, EPOLLIN | EPOLLET | EPOLLRDHUP);
  }
}

void TcpServer::ReceiveData(int socket_fd) {
  while (true) {
    const ssize_t return_value =
        recv(socket_fd, receive_buffer.data(), receive_buffer.size(), 0);
    if (return_value == 0) {
      break;
    }

    if (return_value < 0) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        AddToCloseQueue(socket_fd);
      }
      break;
    }

    OnData(Socket(socket_fd, *this),
           ByteArrayView(receive_buffer.data(),
                         static_cast<std::size_t>(return_value)));
  }
}

void TcpServer::CloseSocket(int socket_fd) {
  epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, socket_fd, nullptr);
  close(socket_fd);
  write_task_table.erase(socket_fd);
}

void TcpServer::AddToCloseQueue(int socket_fd) { close_queue.push(socket_fd); }

void TcpServer::ConsumeCloseQueue() {
  while (!close_queue.empty()) {
    CloseSocket(close_queue.front());
    close_queue.pop();
  }
}

void TcpServer::TryWrite(int socket_fd, const ByteArrayView& data,
                         const std::optional<CallBack>& callback) {
  const auto return_value = send(socket_fd, data.data(), data.size(), 0);

  if (return_value >= 0 &&
      static_cast<std::size_t>(return_value) == data.size()) {
    if (callback) {
      callback.value()();
    }
    return;
  }

  if (return_value < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
    AddToCloseQueue(socket_fd);
    return;
  }

  write_task_table[socket_fd].push(WriteTask{
      .data = ByteArray(data),
      .written_size =
          (return_value > 0) ? static_cast<std::size_t>(return_value) : 0,
      .callback = callback});

  // Add write mask
  AddEvent(socket_fd, EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLOUT, true);
}

void TcpServer::ContinueWrite(int socket_fd) {
  auto& task_queue = write_task_table[socket_fd];

  while (!task_queue.empty()) {
    auto& task = task_queue.front();

    const auto return_value =
        send(socket_fd, std::next(task.data.data(), task.written_size),
             task.data.size() - task.written_size, 0);

    if (return_value >= 0 && static_cast<std::size_t>(return_value) ==
                                 (task.data.size() - task.written_size)) {
      if (task.callback) {
        task.callback.value()();
      }
      task_queue.pop();
      continue;
    }

    if (return_value < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
      AddToCloseQueue(socket_fd);
      break;
    }

    task.written_size +=
        (return_value > 0) ? static_cast<std::size_t>(return_value) : 0;
    break;
  }

  if (task_queue.empty()) {
    // Remove write mask
    AddEvent(socket_fd, EPOLLIN | EPOLLET | EPOLLRDHUP, true);
    return;
  }
}