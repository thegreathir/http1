#ifndef HTTP1_HTTP_SERVER_HPP
#define HTTP1_HTTP_SERVER_HPP

#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

#include "tcp_server.hpp"

namespace http1 {

enum class HttpMethod {
  Unknown,
  Get,
  Head,
  Post,
  Put,
  Delete,
  Connect,
  Options,
  Trace,
  Patch
};

enum class HttpStatusCode {
  // 1xx Informational
  Continue = 100,
  SwitchingProtocols = 101,
  Processing = 102,

  // 2xx Success
  OK = 200,
  Created = 201,
  Accepted = 202,
  NonAuthoritativeInformation = 203,
  NoContent = 204,
  ResetContent = 205,
  PartialContent = 206,
  MultiStatus = 207,
  AlreadyReported = 208,
  IMUsed = 226,

  // 3xx Redirection
  MultipleChoices = 300,
  MovedPermanently = 301,
  Found = 302,
  SeeOther = 303,
  NotModified = 304,
  UseProxy = 305,
  TemporaryRedirect = 307,
  PermanentRedirect = 308,

  // 4xx Client errors
  BadRequest = 400,
  Unauthorized = 401,
  PaymentRequired = 402,
  Forbidden = 403,
  NotFound = 404,
  MethodNotAllowed = 405,
  NotAcceptable = 406,
  ProxyAuthenticationRequired = 407,
  RequestTimeout = 408,
  Conflict = 409,
  Gone = 410,
  LengthRequired = 411,
  PreconditionFailed = 412,
  PayloadTooLarge = 413,
  URITooLong = 414,
  UnsupportedMediaType = 415,
  RangeNotSatisfiable = 416,
  ExpectationFailed = 417,
  ImATeapot = 418,
  MisdirectedRequest = 421,
  UnprocessableEntity = 422,
  Locked = 423,
  FailedDependency = 424,
  TooEarly = 425,
  UpgradeRequired = 426,
  PreconditionRequired = 428,
  TooManyRequests = 429,
  RequestHeaderFieldsTooLarge = 431,
  UnavailableForLegalReasons = 451,

  // 5xx Server errors
  InternalServerError = 500,
  NotImplemented = 501,
  BadGateway = 502,
  ServiceUnavailable = 503,
  GatewayTimeout = 504,
  HTTPVersionNotSupported = 505,
  VariantAlsoNegotiates = 506,
  InsufficientStorage = 507,
  LoopDetected = 508,
  NotExtended = 510,
  NetworkAuthenticationRequired = 511
};

class HttpParseError : public std::invalid_argument {
 public:
  explicit HttpParseError(const std::string& error_message);
};

class HttpSerializeError : public std::invalid_argument {
 public:
  explicit HttpSerializeError(const std::string& error_message);
};

struct HeaderField {
  std::string name;
  std::string value;

  static HeaderField Parse(const std::string_view& data);
};

inline bool operator==(const http1::HeaderField& lhs,
                       const http1::HeaderField& rhs) {
  return lhs.name == rhs.name && lhs.value == rhs.value;
}

using HeaderFields = std::vector<HeaderField>;

class HttpMessage {
 public:
  void AddField(const HeaderField& field);
  void SetBody(const ByteArrayView& body);

  [[nodiscard]] inline const HeaderFields& header_fields() const noexcept {
    return header_fields_;
  }

  [[nodiscard]] inline const std::optional<ByteArrayView>& body()
      const noexcept {
    return body_;
  }

 private:
  HeaderFields header_fields_;
  std::optional<ByteArrayView> body_;
};

class HttpRequest : public HttpMessage {
 public:
  static HttpRequest ParseHeader(const std::string_view& header);
  HttpRequest(HttpMethod method, std::string path, std::string version);
  HttpRequest() = default;

  void UpdateFields(const HeaderField& field);
  void UpdateFields(const std::string& name, const std::string& value);

  [[nodiscard]] inline HttpMethod method() const noexcept { return method_; }

  [[nodiscard]] inline const std::string& path() const noexcept {
    return path_;
  }

  [[nodiscard]] inline const std::string& version() const noexcept {
    return version_;
  }

  [[nodiscard]] inline std::size_t content_length() const noexcept {
    return content_length_;
  }

 private:
  HttpMethod method_ = HttpMethod::Unknown;
  std::string path_;
  std::string version_;

  std::size_t content_length_ = 0;
};

inline bool operator==(const http1::HttpRequest& lhs,
                       const http1::HttpRequest& rhs) {
  return lhs.method() == rhs.method() && lhs.path() == rhs.path() &&
         lhs.version() == rhs.version() &&
         lhs.header_fields() == rhs.header_fields() &&
         lhs.content_length() == rhs.content_length() &&
         lhs.body() == rhs.body();
}

std::ostream& operator<<(std::ostream& output_stream,
                         const HttpRequest& request);

class HttpRequestParser {
 public:
  using RequestCallback = std::function<void(const HttpRequest&)>;
  explicit HttpRequestParser(RequestCallback callback);
  void Feed(const ByteArrayView& data);

  [[nodiscard]] inline const HttpRequest& request() const noexcept {
    return request_;
  }

 private:
  enum class State { 
    BeforeCr1,
    Cr1,
    Lf1,
    Cr2,
    Body
   };

  ByteArray buffer_;
  State state_ = State::BeforeCr1;
  HttpRequest request_;
  RequestCallback on_request_;
};

class HttpResponse : public HttpMessage {
 public:
  explicit HttpResponse(HttpStatusCode status_code);

  void SetReason(const std::string& reason);

  [[nodiscard]] ByteArray Serialize() const;

 private:
  static constexpr const char* VERSION = "HTTP/1.1";

  HttpStatusCode status_code_;
  std::optional<std::string> reason_;
};

class HttpServer : public TcpServer {
 public:
  explicit HttpServer(std::uint16_t port);

 protected:
  virtual HttpResponse OnRequest(const HttpRequest& request) = 0;

 private:
  void OnData(const Socket& socket, const ByteArrayView& data) override;
  void OnClose(const Socket& socket) override;

  std::unordered_map<int, HttpRequestParser> parser_table;
};

}  // namespace http1

#endif