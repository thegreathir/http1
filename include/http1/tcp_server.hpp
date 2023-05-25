#ifndef HTTP1_TCP_SERVER_HPP
#define HTTP1_TCP_SERVER_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace http1 {

class TcpServer {
 public:
  TcpServer(std::uint16_t port);
  void Start();

 protected:
  virtual void OnData(const std::string& data) = 0;

 private:
  static void SetNonBlocking(int socket_fd);
  void AddEvent(int socket_fd, std::uint32_t event_flags);
  void AcceptNewClient();
  bool ReceiveData(int socket_fd);

  static constexpr std::size_t BUFFER_SIZE = 2048;

  const std::uint16_t port_;

  int server_fd_ = -1;
  int epoll_fd_ = -1;

  std::array<std::byte, BUFFER_SIZE> receive_buffer;
};

}  // namespace http1

#endif