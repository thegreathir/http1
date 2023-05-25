#include <iostream>

#include "tcp_server.hpp"

class EchoTcpServer : public http1::TcpServer {
 public:
  EchoTcpServer(std::uint16_t port) : TcpServer(port) {}

 private:
  void OnData(const std::string& data, ReplyStream&& reply) override {
    std::cout << data << std::endl;
    reply << data << std::endl;
    reply.Close();
  }
};

int main() {
  auto server = EchoTcpServer(8000);
  server.Start();
  return 0;
}