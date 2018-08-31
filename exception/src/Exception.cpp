#include "Exception.h"

#include <string>
#include <iostream>

namespace ChimeraTK {

  runtime_error::runtime_error(const std::string &message) noexcept
  : _message(message)
  {}

  const char* runtime_error::what() const noexcept {
    return _message.c_str();
  }

  logic_error::logic_error(const std::string &message) noexcept
  : _message(message)
  {}

  const char* logic_error::what() const noexcept {
    return _message.c_str();
  }

}//namespace ChimeraTK
