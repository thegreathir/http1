#include "http_server.hpp"

#include <gsl/narrow>
#include <iostream>
#include <sstream>
#include <tuple>
#include <utility>

using http1::HeaderField;
using http1::HttpMessage;
using http1::HttpMethod;
using http1::HttpParseError;
using http1::HttpRequest;
using http1::HttpRequestParser;
using http1::HttpResponse;
using http1::HttpSerializeError;
using http1::HttpServer;

HttpMethod ParseMethod(const std::string_view& method) {
  if (method == "GET") {
    return HttpMethod::Get;
  }
  if (method == "HEAD") {
    return HttpMethod::Head;
  }
  if (method == "POST") {
    return HttpMethod::Post;
  }
  if (method == "PUT") {
    return HttpMethod::Put;
  }
  if (method == "DELETE") {
    return HttpMethod::Delete;
  }
  if (method == "CONNECT") {
    return HttpMethod::Connect;
  }
  if (method == "OPTIONS") {
    return HttpMethod::Options;
  }
  if (method == "TRACE") {
    return HttpMethod::Trace;
  }
  if (method == "PATCH") {
    return HttpMethod::Patch;
  }
  throw HttpParseError("Invalid HTTP method");
}

std::string SerializeMethod(HttpMethod method) {
  switch (method) {
    case HttpMethod::Get:
      return "GET";
    case HttpMethod::Head:
      return "HEAD";
    case HttpMethod::Post:
      return "POST";
    case HttpMethod::Put:
      return "PUT";
    case HttpMethod::Delete:
      return "DELETE";
    case HttpMethod::Connect:
      return "CONNECT";
    case HttpMethod::Options:
      return "OPTIONS";
    case HttpMethod::Trace:
      return "TRACE";
    case HttpMethod::Patch:
      return "PATCH";
    default:
      throw HttpSerializeError("Invalid HTTP method");
  }
}

HttpParseError::HttpParseError(const std::string& error_message)
    : std::invalid_argument(error_message) {}

HttpSerializeError::HttpSerializeError(const std::string& error_message)
    : std::invalid_argument(error_message) {}

HeaderField HeaderField::Parse(const std::string_view& data) {
  const std::size_t name_end = data.find(':');
  if (name_end == std::string::npos) {
    throw HttpParseError("Invalid header field");
  }

  auto name = std::string(data.substr(0, name_end));
  const auto value = data.substr(name_end + 1);

  std::transform(
      name.begin(), name.end(), name.begin(),
      [](unsigned char character) { return std::tolower(character); });

  auto trim = [](const std::string_view& input) -> std::string_view {
    const std::size_t first = input.find_first_not_of(" \t");
    const std::size_t last = input.find_last_not_of(" \t");

    if (first == std::string::npos || last == std::string::npos) {
      return "";
    }

    return input.substr(first, last - first + 1);
  };

  return HeaderField{.name = name, .value = std::string(trim(value))};
}

void HttpMessage::AddField(const HeaderField& field) {
  header_fields_.push_back(field);
}

void HttpMessage::SetBody(const ByteArrayView& body) { body_ = body; }

HttpRequest HttpRequest::ParseHeader(const std::string_view& header) {
  const std::size_t request_line_end = header.find("\r\n");
  if (request_line_end == std::string_view::npos) {
    throw HttpParseError("Can not parse request line");
  }

  const std::size_t method_end = header.find(' ');
  if (method_end == std::string_view::npos) {
    throw HttpParseError("Can not parse method from request line");
  }

  const std::size_t path_end = header.find(' ', method_end + 1);
  if (path_end == std::string_view::npos) {
    throw HttpParseError("Can not parse path from request line");
  }

  const std::size_t version_end = header.find("\r\n", path_end + 1);
  if (version_end == std::string_view::npos) {
    throw HttpParseError("Can not parse version from request line");
  }

  auto result = HttpRequest(
      ParseMethod(header.substr(0, method_end)),
      std::string(header.substr(method_end + 1, path_end - method_end - 1)),
      std::string(
          header.substr(path_end + 1, request_line_end - path_end - 1)));

  std::size_t field_start = request_line_end + 2;
  while (field_start < header.size() - 2) {
    const std::size_t field_end = header.find("\r\n", field_start);
    result.UpdateFields(HeaderField::Parse(
        header.substr(field_start, field_end - field_start)));
    field_start = field_end + 2;
  }

  return result;
}

void HttpRequest::UpdateFields(const HeaderField& field) {
  AddField(field);
  if (field.name == "content-length") {
    content_length_ = std::stoi(field.value);
  }
}

void HttpRequest::UpdateFields(const std::string& name,
                               const std::string& value) {
  UpdateFields(HeaderField{.name = name, .value = value});
}

HttpRequest::HttpRequest(HttpMethod method, std::string path,
                         std::string version)
    : method_(method), path_(std::move(path)), version_(std::move(version)) {}

std::ostream& http1::operator<<(std::ostream& output_stream,
                                const HttpRequest& request) {
  output_stream << "Method: \"" << SerializeMethod(request.method()) << "\", "
                << "Path: \"" << request.path() << "\", "
                << "Version: \"" << request.version() << "\"" << std::endl;

  output_stream << "Fields:" << std::endl;
  for (const auto& field : request.header_fields()) {
    output_stream << "{\"" << field.name << "\", \"" << field.value << "\"}"
                  << std::endl;
  }

  return output_stream;
}

HttpRequestParser::HttpRequestParser(RequestCallback callback)
    : on_request_(std::move(callback)) {}

void HttpRequestParser::Feed(const ByteArrayView& data) {
  constexpr auto CARRIAGE_RETURN = std::byte{13};
  constexpr auto LINE_FEED = std::byte{10};

  bool done = false;
  std::size_t consumed = 0;

  for (std::size_t current_it = 0; current_it < data.size() && !done;) {
    const std::byte& current = data[current_it];
    switch (state_) {
      case State::BeforeCr1: {
        if (current == CARRIAGE_RETURN) {
          state_ = State::Cr1;
        }
        break;
      }
      case State::Cr1: {
        if (current == LINE_FEED) {
          state_ = State::Lf1;
        } else if (current != CARRIAGE_RETURN) {
          state_ = State::BeforeCr1;
        }
        break;
      }
      case State::Lf1: {
        if (current == CARRIAGE_RETURN) {
          state_ = State::Cr2;
        } else {
          state_ = State::BeforeCr1;
        }
        break;
      }
      case State::Cr2: {
        if (current == LINE_FEED) {
          std::string_view header;
          if (buffer_.empty()) {
            header = std::string_view{
                reinterpret_cast<const char*>(std::next(
                    data.data(), gsl::narrow<std::int64_t>(consumed))),
                current_it + 1 - consumed};
          } else {
            buffer_.append(data.substr(consumed, current_it + 1 - consumed));
            header = std::string_view{
                reinterpret_cast<const char*>(buffer_.data()), buffer_.size()};
          }
          consumed = current_it + 1;

          request_ = HttpRequest::ParseHeader(header);
          buffer_.clear();

          if (request_.content_length() > 0) {
            state_ = State::Body;
          } else {
            on_request_(request_);

            request_ = HttpRequest{};
            state_ = State::BeforeCr1;
          }
        } else if (current == CARRIAGE_RETURN) {
          state_ = State::Cr1;
        } else {
          state_ = State::BeforeCr1;
        }
        break;
      }
      case State::Body: {
        if ((data.size() - current_it) + buffer_.size() <
            request_.content_length()) {
          done = true;
          break;
        }

        if (buffer_.empty()) {
          consumed += request_.content_length();
          request_.SetBody(data.substr(current_it, request_.content_length()));
        } else {
          consumed += request_.content_length() - buffer_.size();
          buffer_.append(data.substr(
              current_it, request_.content_length() - buffer_.size()));
          request_.SetBody(buffer_);
        }

        current_it = consumed;

        on_request_(request_);
        buffer_.clear();

        request_ = HttpRequest{};
        state_ = State::BeforeCr1;

        break;
      }
    }
    ++current_it;
  }

  buffer_.append(data.substr(consumed));
}

void HttpResponse::SetReason(const std::string& reason) { reason_ = reason; }

HttpResponse::HttpResponse(HttpStatusCode status_code)
    : status_code_(status_code) {}

auto HttpResponse::Serialize() const -> ByteArray {
  std::stringstream header;
  header << VERSION << " " << static_cast<int>(status_code_) << " ";
  if (reason_) {
    header << reason_.value();
  }

  header << "\r\n";

  for (const auto& field : header_fields()) {
    header << field.name << ": " << field.value << "\r\n";
  }

  header << "\r\n";

  const auto header_string = header.str();
  ByteArray result{reinterpret_cast<const std::byte*>(header_string.data()),
                   header_string.size()};
  if (body()) {
    result.append(body().value());
  }

  return result;
}

HttpServer::HttpServer(std::uint16_t port) : TcpServer(port) {};

void HttpServer::OnData(const Socket& socket, const ByteArrayView& data) {
  auto parser_iterator = parser_table.find(socket.socket_fd());
  if (parser_iterator == parser_table.end()) {
    std::tie(parser_iterator, std::ignore) = parser_table.insert(std::make_pair(
        socket.socket_fd(), [this, socket](const HttpRequest& req) {
          socket.Write(OnRequest(req).Serialize());
        }));
  }

  try {
    parser_iterator->second.Feed(data);
    return;
  } catch (const HttpParseError& parse_error) {
    std::cerr << "HTTP request parse failed: " << parse_error.what()
              << std::endl;
  } catch (const HttpSerializeError& serialize_error) {
    std::cerr << "HTTP response serialize failed: " << serialize_error.what()
              << std::endl;
  }

  socket.Close();
}

void HttpServer::OnClose(const Socket& socket) {
  parser_table.erase(socket.socket_fd());
}