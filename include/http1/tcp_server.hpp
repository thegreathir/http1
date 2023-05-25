#ifndef HTTP1_TCP_SERVER_HPP
#define HTTP1_TCP_SERVER_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <queue>
#include <streambuf>
#include <string>

namespace http1 {

class TcpServer {
 public:
  TcpServer(std::uint16_t port);
  virtual ~TcpServer();
  void Start();

 private:
  class ReplyStreamBuffer : public std::streambuf {
   public:
    ReplyStreamBuffer(int socket_fd);

   private:
    std::streamsize xsputn(const char* data,
                           std::streamsize data_size) override;
    int socket_fd_;
  };

 protected:
  class ReplyStream : public std::ostream {
    friend TcpServer;

   public:
    void Close();

   private:
    ReplyStream(ReplyStreamBuffer* stream_buffer, TcpServer* server,
                int socket_fd);
    TcpServer* server_;
    int socket_fd_;
  };

  virtual void OnData(const std::string& data, ReplyStream&& reply_stream) = 0;

 private:
  static void SetNonBlocking(int socket_fd);
  void AddEvent(int socket_fd, std::uint32_t event_flags);
  void AcceptNewClient();
  bool ReceiveData(int socket_fd);
  void CloseSocket(int socket_fd);
  void AddToCloseQueue(int socket_fd);
  void ConsumeCloseQueue();

  static constexpr std::size_t BUFFER_SIZE = 2048;

  const std::uint16_t port_;

  int server_fd_ = -1;
  int epoll_fd_ = -1;

  std::array<std::byte, BUFFER_SIZE> receive_buffer;
  std::queue<int> close_queue;
};

}  // namespace http1

#endif