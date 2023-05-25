#include <iostream>

#include "tcp_server.hpp"

class SampleTcpServer : public http1::TcpServer {
 public:
  SampleTcpServer(std::uint16_t port) : TcpServer(port) {}

 private:
  void OnData(const std::string& data) override {
    std::cout << data << std::endl;
  }
};

int main() {
  auto server = SampleTcpServer(8000);
  server.Start();
  return 0;
}