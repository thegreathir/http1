#include <gtest/gtest.h>

#include <cstring>

#include "http_server.hpp"

constexpr const char* GET_REQUEST =
    "GET / HTTP/1.1"
    "\r\n"
    "Host: 127.0.0.1:8000"
    "\r\n"
    "Connection: keep-alive"
    "\r\n"
    "Cache-Control: max-age=0"
    "\r\n"
    "sec-ch-ua: \"Not:A-Brand\";v=\"99\", \"Chromium\";v=\"112\""
    "\r\n"
    "sec-ch-ua-mobile: ?0"
    "\r\n"
    "sec-ch-ua-platform: \"Linux\""
    "\r\n"
    "Upgrade-Insecure-Requests: 1"
    "\r\n"
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, "
    "like Gecko) Chrome/112.0.0.0 Safari/537.36"
    "\r\n"
    "Accept: "
    "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/"
    "webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7"
    "\r\n"
    "Sec-Fetch-Site: none"
    "\r\n"
    "Sec-Fetch-Mode: navigate"
    "\r\n"
    "Sec-Fetch-User: ?1"
    "\r\n"
    "Sec-Fetch-Dest: document"
    "\r\n"
    "Accept-Encoding: gzip, deflate, br"
    "\r\n"
    "Accept-Language: en-US,en;q=0.9"
    "\r\n"
    "\r\n";
  
class

TEST(RequestParser, SimpleGet) {
  bool called = false;
  auto parser =
      http1::HttpRequestParser([&called](const http1::HttpRequest& req) {
        EXPECT_EQ(http1::HttpMethod::Get, req.method());
        EXPECT_EQ("/", req.path());
        EXPECT_EQ("HTTP/1.1", req.version());
        EXPECT_EQ(15, req.header_fields().size());
        EXPECT_EQ(0, req.content_length());
        called = true;
      });

  parser.Feed(http1::TcpServer::ByteArrayView(
      reinterpret_cast<const std::byte*>(GET_REQUEST),
      std::strlen(GET_REQUEST)));

  EXPECT_TRUE(called);
}