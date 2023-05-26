#include "http_server.hpp"

#include <iostream>

using http1::HeaderField;
using http1::HttpMethod;
using http1::HttpParseError;
using http1::HttpRequest;
using http1::HttpSerializeError;
using http1::HttpServer;
using http1::TcpServer;

HttpMethod ParseMethod(const std::string& method) {
  if (method == "GET")
    return HttpMethod::GET;
  else if (method == "HEAD")
    return HttpMethod::HEAD;
  else if (method == "POST")
    return HttpMethod::POST;
  else if (method == "PUT")
    return HttpMethod::PUT;
  else if (method == "DELETE")
    return HttpMethod::DELETE;
  else if (method == "CONNECT")
    return HttpMethod::CONNECT;
  else if (method == "OPTIONS")
    return HttpMethod::OPTIONS;
  else if (method == "TRACE")
    return HttpMethod::TRACE;
  else if (method == "PATCH")
    return HttpMethod::PATCH;
  else
    throw HttpParseError("Invalid HTTP method: " + method);
}

std::string SerializeMethod(HttpMethod method) {
  switch (method) {
    case HttpMethod::GET:
      return "GET";
    case HttpMethod::HEAD:
      return "HEAD";
    case HttpMethod::POST:
      return "POST";
    case HttpMethod::PUT:
      return "PUT";
    case HttpMethod::DELETE:
      return "DELETE";
    case HttpMethod::CONNECT:
      return "CONNECT";
    case HttpMethod::OPTIONS:
      return "OPTIONS";
    case HttpMethod::TRACE:
      return "TRACE";
    case HttpMethod::PATCH:
      return "PATCH";
    default:
      throw HttpSerializeError("Invalid HTTP method");
  }
}

HttpParseError::HttpParseError(const std::string& error_message)
    : std::invalid_argument(error_message) {}

HttpSerializeError::HttpSerializeError(const std::string& error_message)
    : std::invalid_argument(error_message) {}

HeaderField HeaderField::Parse(const std::string& data) {
  std::size_t name_end = data.find(':');
  if (name_end == std::string::npos) {
    throw HttpParseError("Invalid header field: " + data);
  }

  std::string name = data.substr(0, name_end);
  std::string value = data.substr(name_end + 1);

  std::transform(name.begin(), name.end(), name.begin(),
                 [](unsigned char ch) { return std::tolower(ch); });

  auto trim = [](const std::string& input) -> std::string {
    std::size_t first = input.find_first_not_of(" \t");
    std::size_t last = input.find_last_not_of(" \t");

    if (first == std::string::npos || last == std::string::npos) {
      return "";
    }

    return input.substr(first, last - first + 1);
  };

  return HeaderField{.name = name, .value = trim(value)};
}

void HttpRequest::add_field(const HeaderField& field) {
  header_fields_.push_back(field);
}

void HttpRequest::set_body(const TcpServer::ByteArray& body) { body_ = body; }

HttpRequest HttpRequest::Parse(const TcpServer::ByteArrayView& data) {
  constexpr std::array<std::byte, 4> header_separator = {
      std::byte{13}, std::byte{10}, std::byte{13}, std::byte{10}};

  std::size_t header_end =
      data.find(header_separator.data(), 0, header_separator.size());
  if (header_end == TcpServer::ByteArrayView::npos) {
    throw HttpParseError("Can not find end of header");
  }

  std::string header{reinterpret_cast<const char*>(data.data()), header_end};

  std::size_t request_line_end = header.find("\r\n");
  if (request_line_end == std::string::npos) {
    throw HttpParseError("Can not parse request line: " + header);
  }

  std::size_t method_end = header.find(' ');
  if (method_end == std::string::npos) {
    throw HttpParseError("Can not parse method from request line: " + header);
  }

  std::size_t path_end = header.find(' ', method_end + 1);
  if (path_end == std::string::npos) {
    throw HttpParseError("Can not parse path from request line: " + header);
  }

  std::size_t version_end = header.find("\r\n", path_end + 1);
  if (version_end == std::string::npos) {
    throw HttpParseError("Can not parse version from request line: " + header);
  }

  auto result =
      HttpRequest(ParseMethod(header.substr(0, method_end)),
                  header.substr(method_end + 1, path_end - method_end - 1),
                  header.substr(path_end + 1, request_line_end - path_end - 1));

  std::size_t field_start = request_line_end + 2;
  while (field_start < header_end) {
    std::size_t field_end = header.find("\r\n", field_start);
    if (field_end == std::string::npos) {
      result.add_field(HeaderField::Parse(header.substr(field_start)));
      break;
    }
    result.add_field(HeaderField::Parse(
        header.substr(field_start, field_end - field_start)));
    field_start = field_end + 2;
  }

  if (header_end + header_separator.size() < data.size()) {
    auto body_view = data.substr(header_end + header_separator.size());
    result.set_body(TcpServer::ByteArray(body_view.data(), body_view.size()));
  }

  return result;
}

HttpRequest::HttpRequest(HttpMethod method, const std::string& path,
                         const std::string& version)
    : method_(method), path_(path), version_(version) {}

std::ostream& http1::operator<<(std::ostream& os, const HttpRequest& request) {
  os << "Method: \"" << SerializeMethod(request.method_) << "\", "
     << "Path: \"" << request.path_ << "\", "
     << "Version: \"" << request.version_ << "\"" << std::endl;

  os << "Fields:" << std::endl;
  for (const auto& field : request.header_fields_) {
    os << "{\"" << field.name << "\", \"" << field.value << "\"}" << std::endl;
  }

  return os;
}

HttpServer::HttpServer(std::uint16_t port) : TcpServer(port){};

void HttpServer::OnData(const Socket& socket, const ByteArrayView& data) {
  auto request = HttpRequest::Parse(data);

  std::cout << request << std::endl;
  socket.Close();
}