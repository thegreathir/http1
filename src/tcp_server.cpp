#include "tcp_server.hpp"

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>

#include "syscall_wrapper.hpp"

using http1::TcpServer;

TcpServer::ReplyStreamBuffer::ReplyStreamBuffer(int socket_fd)
    : socket_fd_(socket_fd) {}

std::streamsize TcpServer::ReplyStreamBuffer::xsputn(
    const char* data, std::streamsize data_size) {
  return static_cast<std::streamsize>(
      wrap_syscall(send(socket_fd_, data, data_size, 0),
                   "Error occurred while sending data"));
}

TcpServer::ReplyStream::ReplyStream(ReplyStreamBuffer* stream_buffer,
                                    TcpServer* server, int socket_fd)
    : std::ostream(stream_buffer), server_(server), socket_fd_(socket_fd) {}

void TcpServer::ReplyStream::Close() { server_->AddToCloseQueue(socket_fd_); }

TcpServer::TcpServer(std::uint16_t port) : port_(port) {}

TcpServer::~TcpServer() {
  close(epoll_fd_);
  close(server_fd_);
}

void TcpServer::Start() {
  server_fd_ = wrap_syscall(socket(AF_INET, SOCK_STREAM, 0),
                            "Can not create TCP socket");

  int option_on = 1;
  wrap_syscall(setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &option_on,
                          sizeof(option_on)),
               "Can not set address reuse options");

  SetNonBlocking(server_fd_);

  sockaddr_in server_address{};
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(port_);

  wrap_syscall(bind(server_fd_, reinterpret_cast<sockaddr*>(&server_address),
                    sizeof(server_address)),
               "Can not bind server socket");
  wrap_syscall(listen(server_fd_, SOMAXCONN), "Can not start listening");

  epoll_fd_ = wrap_syscall(epoll_create(1), "Can create epoll");

  AddEvent(server_fd_, EPOLLIN | EPOLLOUT | EPOLLET);

  constexpr int MAX_EPOLL_EVENTS = 64;
  std::array<epoll_event, MAX_EPOLL_EVENTS> epoll_event_list;
  while (true) {
    const int number_of_fds = wrap_syscall(
        epoll_wait(epoll_fd_, epoll_event_list.data(), MAX_EPOLL_EVENTS, -1),
        "Error occurred while waiting for new events");
    for (int fd_iterator = 0; fd_iterator < number_of_fds; ++fd_iterator) {
      auto& current_event = epoll_event_list[fd_iterator];
      if (current_event.data.fd == server_fd_) {
        AcceptNewClient();
      } else if (current_event.events & EPOLLIN) {
        while (ReceiveData(current_event.data.fd)) {
        }
      }

      if (current_event.events & EPOLLHUP ||
          current_event.events & EPOLLRDHUP) {
        AddToCloseQueue(current_event.data.fd);
      }
    }

    ConsumeCloseQueue();
  }
}

void TcpServer::SetNonBlocking(int socket_fd) {
  const int DEFAULT_FLAGS =
      wrap_syscall(fcntl(socket_fd, F_GETFL, 0), "Can not get socket options");
  wrap_syscall(fcntl(socket_fd, F_SETFL, DEFAULT_FLAGS | O_NONBLOCK),
               "Can not enable non-blocking for socket");
}

void TcpServer::AddEvent(int socket_fd, std::uint32_t event_flags) {
  epoll_event event;
  event.events = event_flags;
  event.data.fd = socket_fd;
  wrap_syscall(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket_fd, &event),
               "Can not add socket to epoll");
}

void TcpServer::AcceptNewClient() {
  int new_client_fd = wrap_syscall(accept(server_fd_, nullptr, nullptr),
                                   "Can not accept new connection");

  SetNonBlocking(new_client_fd);
  AddEvent(new_client_fd, EPOLLIN | EPOLLET | EPOLLRDHUP);
}

bool TcpServer::ReceiveData(int socket_fd) {
  ssize_t return_value = recv(socket_fd, receive_buffer.data(), BUFFER_SIZE, 0);
  if (return_value == 0) {
    return false;
  }

  if (return_value < 0) {
    if (errno != EAGAIN) {
      wrap_syscall(return_value, "Read error");
    }
    return false;
  }

  ReplyStreamBuffer stream_buffer(socket_fd);
  OnData(std::string(reinterpret_cast<const char*>(receive_buffer.data()),
                     static_cast<std::size_t>(return_value)),
         ReplyStream(&stream_buffer, this, socket_fd));
  return true;
}

void TcpServer::CloseSocket(int socket_fd) {
  epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, socket_fd, nullptr);
  close(socket_fd);
}

void TcpServer::AddToCloseQueue(int socket_fd) { close_queue.push(socket_fd); }

void TcpServer::ConsumeCloseQueue() {
  while (!close_queue.empty()) {
    CloseSocket(close_queue.front());
    close_queue.pop();
  }
}