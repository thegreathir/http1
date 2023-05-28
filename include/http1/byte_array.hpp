#ifndef HTTP1_BYTE_ARRAY_HPP
#define HTTP1_BYTE_ARRAY_HPP

#include <string>

namespace http1 {

using ByteArray = std::basic_string<std::byte>;
using ByteArrayView = std::basic_string_view<std::byte>;

}  // namespace http1

#endif