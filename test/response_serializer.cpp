#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "http_server.hpp"

constexpr const char* OK_RESPONSE =
    "HTTP/1.1 200 OK"
    "\r\n"
    "Server: nginx/1.22.1"
    "\r\n"
    "Date: Sun, 28 May 2023 10:57:01 GMT"
    "\r\n"
    "Content-Type: text/html"
    "\r\n"
    "Content-Length: 615"
    "\r\n"
    "Last-Modified: Tue, 01 Nov 2022 21:46:23 GMT"
    "\r\n"
    "Connection: keep-alive"
    "\r\n"
    "ETag: \"636193af-267\""
    "\r\n"
    "Accept-Ranges: bytes"
    "\r\n"
    "\r\n";

constexpr const char* OK_BODY =
    "<!DOCTYPE html>"
    "\n"
    "<html>"
    "\n"
    "<head>"
    "\n"
    "<title>Welcome to nginx!</title>"
    "\n"
    "<style>"
    "\n"
    "html { color-scheme: light dark; }"
    "\n"
    "body { width: 35em; margin: 0 auto;"
    "\n"
    "font-family: Tahoma, Verdana, Arial, sans-serif; }"
    "\n"
    "</style>"
    "\n"
    "</head>"
    "\n"
    "<body>"
    "\n"
    "<h1>Welcome to nginx!</h1>"
    "\n"
    "<p>If you see this page, the nginx web server is successfully installed "
    "and"
    "\n"
    "working. Further configuration is required.</p>"
    "\n"
    "\n"
    "<p>For online documentation and support please refer to"
    "\n"
    "<a href=\"http://nginx.org/\">nginx.org</a>.<br/>"
    "\n"
    "Commercial support is available at"
    "\n"
    "<a href=\"http://nginx.com/\">nginx.com</a>.</p>"
    "\n"
    "\n"
    "<p><em>Thank you for using nginx.</em></p>"
    "\n"
    "</body>"
    "\n"
    "</html>"
    "\n";

TEST(ResponseSerializer, SimpleResponse) {
  http1::HttpResponse response{http1::HttpStatusCode::OK};

  response.AddField(
      http1::HeaderField{.name = "Server", .value = "nginx/1.22.1"});
  response.AddField(http1::HeaderField{
      .name = "Date", .value = "Sun, 28 May 2023 10:57:01 GMT"});
  response.AddField(
      http1::HeaderField{.name = "Content-Type", .value = "text/html"});
  response.AddField(
      http1::HeaderField{.name = "Content-Length", .value = "615"});
  response.AddField(http1::HeaderField{
      .name = "Last-Modified", .value = "Tue, 01 Nov 2022 21:46:23 GMT"});
  response.AddField(
      http1::HeaderField{.name = "Connection", .value = "keep-alive"});
  response.AddField(
      http1::HeaderField{.name = "ETag", .value = "\"636193af-267\""});
  response.AddField(
      http1::HeaderField{.name = "Accept-Ranges", .value = "bytes"});

  response.SetReason("OK");

  response.SetBody(http1::ByteArrayView(
      reinterpret_cast<const std::byte*>(OK_BODY), std::strlen(OK_BODY)));

  std::string data = std::string(OK_RESPONSE) + OK_BODY;

  EXPECT_EQ(http1::ByteArray(reinterpret_cast<const std::byte*>(data.c_str()),
                             data.size()),
            response.Serialize());
}