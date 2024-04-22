// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Exception.h"

#include <iostream>
#include <string>
#include <utility>

namespace ChimeraTK {

  runtime_error::runtime_error(std::string message) noexcept : _message(std::move(message)) {}

  const char* runtime_error::what() const noexcept {
    return _message.c_str();
  }

  logic_error::logic_error(std::string message) noexcept : _message(std::move(message)) {}

  const char* logic_error::what() const noexcept {
    return _message.c_str();
  }

} // namespace ChimeraTK
