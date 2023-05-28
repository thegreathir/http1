#ifndef HTTP1_BYTE_ARRAY_H
#define HTTP1_BYTE_ARRAY_H

#include <string>

namespace http1 {

using ByteArray = std::basic_string<std::byte>;
using ByteArrayView = std::basic_string_view<std::byte>;

}  // namespace http1

#endif