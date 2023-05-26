#ifndef HTTP1_TCP_SERVER_HPP
#define HTTP1_TCP_SERVER_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <ostream>
#include <queue>
#include <streambuf>
#include <string>
#include <string_view>

namespace http1 {

class TcpServer {
 public:
  explicit TcpServer(std::uint16_t port);
  virtual ~TcpServer();

  TcpServer(const TcpServer& other) = delete;
  TcpServer(TcpServer&& other) = delete;

  TcpServer& operator=(const TcpServer& other) = delete;
  TcpServer& operator=(TcpServer&& other) = delete;

  void Start();

 protected:
  using ByteArray = std::basic_string<std::byte>;
  using ByteArrayView = std::basic_string_view<std::byte>;

  using CallBack = std::function<void()>;

  class Socket {
    friend TcpServer;

   public:
    bool Write(const ByteArray& data,
               std::optional<CallBack> callback = std::nullopt) const;
    void Close() const;

   private:
    Socket(int socket_fd, TcpServer* server);
    int socket_fd_;
    TcpServer* server_;
  };

  virtual void OnData(const Socket& socket, const ByteArrayView& data) = 0;

 private:
  struct WriteTask {
    ByteArray data;
    std::size_t written_size;
    std::optional<CallBack> callback;
  };

  static void SetNonBlocking(int socket_fd);
  void AddEvent(int socket_fd, std::uint32_t event_flags,
                bool update = false) const;
  void AcceptNewClient();
  bool ReceiveData(int socket_fd);
  void CloseSocket(int socket_fd);
  void AddToCloseQueue(int socket_fd);
  void ConsumeCloseQueue();
  bool TryWrite(int socket_fd, const ByteArray& data,
                std::optional<CallBack> callback = std::nullopt);
  
  void ContinueWrite(int socket_fd);

  static constexpr std::size_t BUFFER_SIZE = 2048;

  const std::uint16_t port_;

  int server_fd_ = -1;
  int epoll_fd_ = -1;

  std::array<std::byte, BUFFER_SIZE> receive_buffer = {};
  std::queue<int> close_queue;

  std::unordered_map<int, std::queue<WriteTask>> write_task_table;
};

}  // namespace http1

#endif