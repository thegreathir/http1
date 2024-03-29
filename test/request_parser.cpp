#include <gtest/gtest.h>

#include <cstring>
#include <memory>

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

constexpr const char* GET_REQUEST_WITHOUT_FIELDS =
    "GET / HTTP/1.1"
    "\r\n"
    "\r\n";

http1::HttpRequest expected_get_request() {
  auto result = http1::HttpRequest(http1::HttpMethod::Get, "/", "HTTP/1.1");
  result.UpdateFields("host", "127.0.0.1:8000");
  result.UpdateFields("connection", "keep-alive");
  result.UpdateFields("cache-control", "max-age=0");
  result.UpdateFields("sec-ch-ua",
                      "\"Not:A-Brand\";v=\"99\", \"Chromium\";v=\"112\"");
  result.UpdateFields("sec-ch-ua-mobile", "?0");
  result.UpdateFields("sec-ch-ua-platform", "\"Linux\"");
  result.UpdateFields("upgrade-insecure-requests", "1");
  result.UpdateFields("user-agent",
                      "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
                      "(KHTML, like Gecko) Chrome/112.0.0.0 Safari/537.36");
  result.UpdateFields(
      "accept",
      "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/"
      "webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7");
  result.UpdateFields("sec-fetch-site", "none");
  result.UpdateFields("sec-fetch-mode", "navigate");
  result.UpdateFields("sec-fetch-user", "?1");
  result.UpdateFields("sec-fetch-dest", "document");
  result.UpdateFields("accept-encoding", "gzip, deflate, br");
  result.UpdateFields("accept-language", "en-US,en;q=0.9");

  return result;
}

constexpr const char* POST_REQUEST =
    "POST / HTTP/1.1"
    "\r\n"
    "Content-Type: application/json"
    "\r\n"
    "User-Agent: PostmanRuntime/7.29.3"
    "\r\n"
    "Accept: */*"
    "\r\n"
    "Postman-Token: 3cbd5d2b-758d-4b5b-93c8-d5d672445aed"
    "\r\n"
    "Host: 127.0.0.1:8000"
    "\r\n"
    "Accept-Encoding: gzip, deflate, br"
    "\r\n"
    "Connection: keep-alive"
    "\r\n"
    "Content-Length: 119"
    "\r\n"
    "\r\n";

constexpr const char* POST_REQUEST_BODY =
    "{"
    "\n"
    "    \"key1\": [1, 2, 3],"
    "\n"
    "    \"key2\": {"
    "\n"
    "        \"k1\": false,"
    "\n"
    "        \"k2\": [\"str1\", \"str2\"]"
    "\n"
    "    },"
    "\n"
    "    \"key3\": \"value\""
    "\n"
    "}";

http1::HttpRequest expected_post_request() {
  auto result = http1::HttpRequest(http1::HttpMethod::Post, "/", "HTTP/1.1");
  result.UpdateFields("content-type", "application/json");
  result.UpdateFields("user-agent", "PostmanRuntime/7.29.3");
  result.UpdateFields("accept", "*/*");
  result.UpdateFields("postman-token", "3cbd5d2b-758d-4b5b-93c8-d5d672445aed");
  result.UpdateFields("host", "127.0.0.1:8000");
  result.UpdateFields("accept-encoding", "gzip, deflate, br");
  result.UpdateFields("connection", "keep-alive");
  result.UpdateFields("content-length", "119");

  result.SetBody(http1::ByteArrayView(
      reinterpret_cast<const std::byte*>(POST_REQUEST_BODY),
      std::strlen(POST_REQUEST_BODY)));
  return result;
}

class RequestParserTest : public testing::Test {
 public:
  void SetUp() override {
    parser_ = std::make_unique<http1::HttpRequestParser>(
        [this](const http1::HttpRequest& req) { OnRequest(req); });
  }

  void OnRequest(const http1::HttpRequest& req) {
    EXPECT_EQ(expected_requests_[expected_request_index_++], req);
    parsed_requests_.push_back(req);
  }

  void Feed(const std::string& data) {
    parser_->Feed(http1::ByteArrayView(
        reinterpret_cast<const std::byte*>(data.data()), data.size()));
  }

  std::vector<http1::HttpRequest> expected_requests_;
  std::vector<http1::HttpRequest> parsed_requests_;
  std::size_t expected_request_index_ = 0;

  std::unique_ptr<http1::HttpRequestParser> parser_;
};

// Simple GET
TEST_F(RequestParserTest, SimpleGet) {
  expected_requests_.push_back(expected_get_request());
  Feed(GET_REQUEST);

  EXPECT_EQ(1, expected_request_index_);
}

TEST_F(RequestParserTest, SimpleGetBytePerByte) {
  expected_requests_.push_back(expected_get_request());

  for (char byte : std::string(GET_REQUEST)) {
    Feed(std::string(1, byte));
  }

  EXPECT_EQ(1, expected_request_index_);
}

TEST_F(RequestParserTest, SimpleGetThreeBytesPerThreeBytes) {
  expected_requests_.push_back(expected_get_request());

  for (std::size_t i = 0; i < std::strlen(GET_REQUEST); i += 3) {
    auto chunk = std::string(GET_REQUEST).substr(i, 3);
    Feed(chunk);
  }

  EXPECT_EQ(1, expected_request_index_);
}

// Simple GET without any fields
TEST_F(RequestParserTest, SimpleGetWithoutFields) {
  expected_requests_.push_back(
      http1::HttpRequest::ParseHeader(GET_REQUEST_WITHOUT_FIELDS));
  Feed(GET_REQUEST_WITHOUT_FIELDS);

  EXPECT_EQ(1, expected_request_index_);
}

TEST_F(RequestParserTest, SimpleGetWithoutFieldsBytePerByte) {
  expected_requests_.push_back(
      http1::HttpRequest::ParseHeader(GET_REQUEST_WITHOUT_FIELDS));

  for (char byte : std::string(GET_REQUEST_WITHOUT_FIELDS)) {
    Feed(std::string(1, byte));
  }

  EXPECT_EQ(1, expected_request_index_);
}

TEST_F(RequestParserTest, SimpleGetWithoutFieldsThreeBytesPerThreeBytes) {
  expected_requests_.push_back(
      http1::HttpRequest::ParseHeader(GET_REQUEST_WITHOUT_FIELDS));

  for (std::size_t i = 0; i < std::strlen(GET_REQUEST_WITHOUT_FIELDS); i += 3) {
    auto chunk = std::string(GET_REQUEST_WITHOUT_FIELDS).substr(i, 3);
    Feed(chunk);
  }

  EXPECT_EQ(1, expected_request_index_);
}

// Simple POST
TEST_F(RequestParserTest, SimplePost) {
  expected_requests_.push_back(expected_post_request());
  Feed(std::string(POST_REQUEST) + POST_REQUEST_BODY);
  ASSERT_EQ(1, parsed_requests_.size());
  ASSERT_TRUE(parsed_requests_.back().body().has_value());

  EXPECT_EQ(119, parsed_requests_.back().body().value().size());
}

TEST_F(RequestParserTest, SimplePostThreeBytesPerThreeBytes) {
  expected_requests_.push_back(expected_post_request());

  std::string post = std::string(POST_REQUEST) + POST_REQUEST_BODY;
  for (std::size_t i = 0; i < post.size(); i += 3) {
    Feed(post.substr(i, 3));
  }

  EXPECT_EQ(1, expected_request_index_);
}

TEST_F(RequestParserTest, SimplePostBytePerByte) {
  expected_requests_.push_back(expected_post_request());

  for (char byte : std::string(POST_REQUEST) + POST_REQUEST_BODY) {
    Feed(std::string(1, byte));
  }

  EXPECT_EQ(1, expected_request_index_);
}

// Two consecutive requests
TEST_F(RequestParserTest, FirstGetSecondPost) {
  expected_requests_.push_back(expected_get_request());
  expected_requests_.push_back(expected_post_request());

  auto data = std::string(GET_REQUEST) + POST_REQUEST + POST_REQUEST_BODY;
  Feed(data);

  EXPECT_EQ(2, expected_request_index_);
}

TEST_F(RequestParserTest, FirstGetSecondPostSeparated) {
  expected_requests_.push_back(expected_get_request());
  expected_requests_.push_back(expected_post_request());

  auto data1 = std::string(GET_REQUEST);
  auto data2 = std::string(POST_REQUEST) + POST_REQUEST_BODY;
  Feed(data1);
  Feed(data2);

  EXPECT_EQ(2, expected_request_index_);
}

TEST_F(RequestParserTest, FirstPostSecondGet) {
  expected_requests_.push_back(expected_post_request());
  expected_requests_.push_back(expected_get_request());

  auto data = std::string(POST_REQUEST) + POST_REQUEST_BODY + GET_REQUEST;
  Feed(data);

  EXPECT_EQ(2, expected_request_index_);
}

TEST_F(RequestParserTest, FirstPostSecondGetSeparated) {
  expected_requests_.push_back(expected_post_request());
  expected_requests_.push_back(expected_get_request());

  auto data1 = std::string(GET_REQUEST);
  auto data2 = std::string(POST_REQUEST) + POST_REQUEST_BODY;
  Feed(data2);
  Feed(data1);

  EXPECT_EQ(2, expected_request_index_);
}

TEST_F(RequestParserTest, GetAndPostThreeBytesPerThreeBytes) {
  expected_requests_.push_back(expected_post_request());
  expected_requests_.push_back(expected_get_request());

  auto data = std::string(POST_REQUEST) + POST_REQUEST_BODY + GET_REQUEST;
  for (std::size_t i = 0; i < data.size(); i += 3) {
    Feed(data.substr(i, 3));
  }

  EXPECT_EQ(2, expected_request_index_);
}

TEST_F(RequestParserTest, GetAndPostBytePerByte) {
  expected_requests_.push_back(expected_post_request());
  expected_requests_.push_back(expected_get_request());

  for (char byte :
       std::string(POST_REQUEST) + POST_REQUEST_BODY + GET_REQUEST) {
    Feed(std::string(1, byte));
  }

  EXPECT_EQ(2, expected_request_index_);
}

TEST_F(RequestParserTest, IncompleteGetThenPost) {
  for (int i = 0; i < 17; ++i) {
    expected_requests_.push_back(expected_get_request());
    expected_requests_.push_back(expected_post_request());

    std::string data1 =
        std::string(GET_REQUEST).substr(0, std::strlen(GET_REQUEST) - i);
    std::string data2 =
        std::string(GET_REQUEST).substr(std::strlen(GET_REQUEST) - i, i) +
        POST_REQUEST + POST_REQUEST_BODY;

    Feed(data1);
    Feed(data2);

    EXPECT_EQ(2, expected_request_index_);

    expected_request_index_ = 0;
    expected_requests_.clear();
  }
}

TEST_F(RequestParserTest, IncompleteGetThenTwoRequests) {
  for (int i = 0; i < 17; ++i) {
    expected_requests_.push_back(expected_get_request());
    expected_requests_.push_back(expected_post_request());
    expected_requests_.push_back(expected_post_request());

    std::string data1 =
        std::string(GET_REQUEST).substr(0, std::strlen(GET_REQUEST) - i);
    std::string data2 =
        std::string(GET_REQUEST).substr(std::strlen(GET_REQUEST) - i, i) +
        POST_REQUEST + POST_REQUEST_BODY + POST_REQUEST + POST_REQUEST_BODY;

    Feed(data1);
    Feed(data2);

    EXPECT_EQ(3, expected_request_index_);

    expected_request_index_ = 0;
    expected_requests_.clear();
  }
}

TEST_F(RequestParserTest, AllPossibleTwoChunksOfTwoRequests) {
  std::string data =
      std::string(GET_REQUEST) + POST_REQUEST + POST_REQUEST_BODY;
  for (std::size_t i = 0; i < data.size(); ++i) {
    expected_requests_.push_back(expected_get_request());
    expected_requests_.push_back(expected_post_request());

    Feed(data.substr(0, i));
    Feed(data.substr(i));

    EXPECT_EQ(2, expected_request_index_);

    expected_request_index_ = 0;
    expected_requests_.clear();
  }
}

TEST_F(RequestParserTest, AllPossibleTwoChunksOfThreeRequests) {
  std::string data =
      std::string(GET_REQUEST) + POST_REQUEST + POST_REQUEST_BODY + GET_REQUEST;
  for (std::size_t i = 0; i < data.size(); ++i) {
    expected_requests_.push_back(expected_get_request());
    expected_requests_.push_back(expected_post_request());
    expected_requests_.push_back(expected_get_request());

    Feed(data.substr(0, i));
    Feed(data.substr(i));

    EXPECT_EQ(3, expected_request_index_);

    expected_request_index_ = 0;
    expected_requests_.clear();
  }
}

TEST_F(RequestParserTest, AllPossibleThreeChunksOfTwoRequests) {
  std::string data =
      std::string(GET_REQUEST) + POST_REQUEST + POST_REQUEST_BODY;
  for (std::size_t i = 0; i < data.size(); ++i) {
    for (std::size_t j = 0; j < i; ++j) {
      expected_requests_.push_back(expected_get_request());
      expected_requests_.push_back(expected_post_request());

      Feed(data.substr(0, j));
      Feed(data.substr(j, i - j));
      Feed(data.substr(i));

      EXPECT_EQ(2, expected_request_index_);

      expected_request_index_ = 0;
      expected_requests_.clear();
    }
  }
}

TEST_F(RequestParserTest, AllPossibleThreeChunksOfThreeRequests) {
  std::string data =
      std::string(GET_REQUEST) + POST_REQUEST + POST_REQUEST_BODY + GET_REQUEST;
  for (std::size_t i = 0; i < data.size(); ++i) {
    for (std::size_t j = 0; j < i; ++j) {
      expected_requests_.push_back(expected_get_request());
      expected_requests_.push_back(expected_post_request());
      expected_requests_.push_back(expected_get_request());

      Feed(data.substr(0, j));
      Feed(data.substr(j, i - j));
      Feed(data.substr(i));

      EXPECT_EQ(3, expected_request_index_);

      expected_request_index_ = 0;
      expected_requests_.clear();
    }
  }
}
