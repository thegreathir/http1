#include <iostream>

#include "tcp_server.hpp"

class SampleTcpServer : public http1::TcpServer {
 public:
  SampleTcpServer(std::uint16_t port) : TcpServer(port) {}

 private:
  void OnData(const ConstWeakBuffer& buffer) override {
    std::cout << std::string(reinterpret_cast<const char*>(buffer.data),
                             buffer.data_size)
              << std::endl;
  }
};

int main() {
  auto server = SampleTcpServer(8000);
  server.Start();
  return 0;
}