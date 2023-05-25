#include <iostream>

#include "tcp_server.hpp"

class EchoTcpServer : public http1::TcpServer {
 public:
  explicit EchoTcpServer(std::uint16_t port) : TcpServer(port) {}

 private:
  void OnData(const std::string& data, ReplyStream&& reply) override {
    std::cout << data << std::endl;
    reply << data << std::endl;
    reply.Close();
  }
};

int main() {
  constexpr std::uint16_t DEFAULT_PORT = 8000;
  auto server = EchoTcpServer(DEFAULT_PORT);
  server.Start();
  return 0;
}