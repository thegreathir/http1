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

  static HeaderField Parse(const std::string& data);
};

using HeaderFields = std::vector<HeaderField>;

class HttpRequest {
  friend std::ostream& operator<<(std::ostream& os, const HttpRequest& request);

 public:
  static HttpRequest Parse(const TcpServer::ByteArrayView& data);
  HttpRequest(HttpMethod method, const std::string& path,
              const std::string& version);

  void add_field(const HeaderField& field);
  void set_body(const TcpServer::ByteArray& body);

 private:
  HttpMethod method_;
  std::string path_;
  std::string version_;

  HeaderFields header_fields_;
  std::optional<TcpServer::ByteArray> body_;
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