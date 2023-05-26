#include <iostream>

#include "tcp_server.hpp"

class EchoTcpServer : public http1::TcpServer {
 public:
  explicit EchoTcpServer(std::uint16_t port) : TcpServer(port) {}

 private:
  void OnData(const Socket& socket, const ByteArrayView& data) override {
    socket.Write(ByteArray(data.data(), data.size()));
  }
};

int main() {
  constexpr std::uint16_t DEFAULT_PORT = 8000;
  auto server = EchoTcpServer(DEFAULT_PORT);
  server.Start();
  return 0;
}