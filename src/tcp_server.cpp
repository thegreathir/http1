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

TcpServer::TcpServer(std::uint16_t port) : port_(port) {}

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

  constexpr int MAX_EPOLL_EVENTS = 64;
  std::array<epoll_event, MAX_EPOLL_EVENTS> epoll_event_list{};
  while (true) {
    const int number_of_fds = wrap_syscall(
        epoll_wait(epoll_fd_, epoll_event_list.data(), MAX_EPOLL_EVENTS, -1),
        "Error occurred while waiting for new events");
    for (int fd_iterator = 0; fd_iterator < number_of_fds; ++fd_iterator) {
      auto& current_event = epoll_event_list.at(fd_iterator);
      if (current_event.data.fd == server_fd_) {
        while (AcceptNewClient()) {
        }
      } else {
        if ((current_event.events & EPOLLIN) != 0U) {
          while (ReceiveData(current_event.data.fd)) {
          }
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
  const unsigned int DEFAULT_FLAGS =
      wrap_syscall(fcntl(socket_fd, F_GETFL, 0), "Can not get socket flags");

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

bool TcpServer::AcceptNewClient() {
  const int new_client_fd = accept(server_fd_, nullptr, nullptr);

  if (new_client_fd < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      wrap_syscall(new_client_fd, "Can not accept new connection");
    }
    return false;
  }

  SetNonBlocking(new_client_fd);
  AddEvent(new_client_fd, EPOLLIN | EPOLLET | EPOLLRDHUP);
  return true;
}

bool TcpServer::ReceiveData(int socket_fd) {
  const ssize_t return_value =
      recv(socket_fd, receive_buffer.data(), BUFFER_SIZE, 0);
  if (return_value == 0) {
    return false;
  }

  if (return_value < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      wrap_syscall(return_value, "Read error");
    }
    return false;
  }

  OnData(Socket(socket_fd, *this),
         ByteArrayView(receive_buffer.data(),
                       static_cast<std::size_t>(return_value)));

  return true;
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
    wrap_syscall(return_value, "Write error");
    return;
  }

  write_task_table[socket_fd].push(WriteTask{
      .data = ByteArray(data),
      .written_size =
          (return_value > 0) ? static_cast<std::size_t>(return_value) : 0,
      .callback = callback});

  // Add write mask
  AddEvent(socket_fd, EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLOUT, true);
  return;
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
      wrap_syscall(return_value, "Write error");
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