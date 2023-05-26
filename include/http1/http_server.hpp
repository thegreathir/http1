#ifndef HTTP1_HTTP_SERVER_HPP
#define HTTP1_HTTP_SERVER_HPP

#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "tcp_server.hpp"

namespace http1 {

enum class HttpMethod {
  GET,
  HEAD,
  POST,
  PUT,
  DELETE,
  CONNECT,
  OPTIONS,
  TRACE,
  PATCH
};

class HttpParseError : std::invalid_argument {
 public:
  HttpParseError(const std::string& error_message);
};

class HttpSerializeError : std::invalid_argument {
 public:
  HttpSerializeError(const std::string& error_message);
};

struct HeaderField {
  std::string name;
  std::string value;
};

using HeaderFields = std::vector<HeaderField>;

class HttpRequest {
  friend std::ostream& operator<<(std::ostream& os, const HttpRequest& request);
 public:
  static HttpRequest Parse(const TcpServer::ByteArrayView& data);
  HttpRequest(HttpMethod method, const std::string& path,
              const std::string& version);

 private:
  HttpMethod method_;
  std::string path_;
  std::string version_;

  HeaderFields header_fields_;
  std::optional<std::string> body_;
};

std::ostream& operator<<(std::ostream& os, const HttpRequest& request);

class HttpServer : public TcpServer {
 public:
  explicit HttpServer(std::uint16_t port);

 private:
  void OnData(const Socket& socket, const ByteArrayView& data) override;
};

}  // namespace http1

#endif