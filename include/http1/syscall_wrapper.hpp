#ifndef HTTP1_SYSCALL_WRAPPER_HPP
#define HTTP1_SYSCALL_WRAPPER_HPP

#include <system_error>

namespace http1 {

template <class T = int>
inline T wrap_syscall(T return_value, const char* error_message) {
  if (return_value < 0) {
    throw std::system_error(errno, std::generic_category(), error_message);
  }

  return return_value;
}

}  // namespace http1

#endif