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
  static constexpr std::size_t DEFAULT_BUFFER_SIZE = 2048;
  using ByteArray = std::basic_string<std::byte>;
  using ByteArrayView = std::basic_string_view<std::byte>;

  explicit TcpServer(std::uint16_t port,
                     std::size_t receive_buffer_size = DEFAULT_BUFFER_SIZE);
  virtual ~TcpServer();

  TcpServer(const TcpServer& other) = delete;
  TcpServer(TcpServer&& other) = delete;

  TcpServer& operator=(const TcpServer& other) = delete;
  TcpServer& operator=(TcpServer&& other) = delete;

  void Start();

 protected:
  using CallBack = std::function<void()>;

  class Socket {
    friend TcpServer;

   public:
    void Write(const ByteArrayView& data,
               const std::optional<CallBack>& callback = std::nullopt) const;
    void Close() const;

    [[nodiscard]] inline int socket_fd() const noexcept { return socket_fd_; }

   private:
    Socket(int socket_fd, TcpServer& server);
    int socket_fd_;

    TcpServer& server_;
  };

  virtual void OnData(const Socket& socket, const ByteArrayView& data) = 0;
  virtual void OnClose(const Socket& socket) = 0;

 private:
  struct WriteTask {
    ByteArray data;
    std::size_t written_size;
    std::optional<CallBack> callback;
  };

  static void SetNonBlocking(int socket_fd);
  void AddEvent(int socket_fd, std::uint32_t event_flags,
                bool update = false) const;
  void LoopEvents();
  void AcceptNewClients();
  void ReceiveData(int socket_fd);
  void CloseSocket(int socket_fd);
  void AddToCloseQueue(int socket_fd);
  void ConsumeCloseQueue();
  void TryWrite(int socket_fd, const ByteArrayView& data,
                const std::optional<CallBack>& callback = std::nullopt);

  void ContinueWrite(int socket_fd);

  const std::uint16_t port_;

  int server_fd_ = -1;
  int epoll_fd_ = -1;

  ByteArray receive_buffer = {};
  std::queue<int> close_queue;

  std::unordered_map<int, std::queue<WriteTask>> write_task_table;
};

}  // namespace http1

#endif