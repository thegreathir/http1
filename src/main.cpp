#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "http_server.hpp"

class ExampleHttpServer : public http1::HttpServer {
 public:
  explicit ExampleHttpServer(std::uint16_t port) : HttpServer(port) {
    index = open_file("index.html");
    background = open_file("bg.jpg");
  }

 private:
  static ByteArray open_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
      throw std::invalid_argument("Can not open file: " + path);
    }

    file.seekg(0, std::ios::end);
    const std::size_t size = file.tellg();

    file.seekg(0, std::ios::beg);

    ByteArray content(size, std::byte{0});

    file.read(reinterpret_cast<char*>(content.data()), size);
    file.close();

    return content;
  }

  http1::HttpResponse OnRequest(const http1::HttpRequest& req) override {
    http1::HttpResponse res(http1::HttpStatusCode::OK);
    if (req.GetPath() == "/") {
      res.SetBody(index);
      res.AddField(http1::HeaderField{.name = "content-length",
                                      .value = std::to_string(index.size())});
      res.AddField(http1::HeaderField{.name = "content-type",
                                      .value = "text/html; charset=UTF-8"});
    } else if (req.GetPath() == "/bg.jpg") {
      res.SetBody(background);
      res.AddField(
          http1::HeaderField{.name = "content-length",
                             .value = std::to_string(background.size())});
      res.AddField(
          http1::HeaderField{.name = "content-type", .value = "image/jpeg"});
    }

    return res;
  }

  ByteArray index;
  ByteArray background;
};

int main() {
  constexpr std::uint16_t DEFAULT_PORT = 8000;
  auto server = ExampleHttpServer(DEFAULT_PORT);
  server.Start();
  return 0;
}