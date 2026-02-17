// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

// Note: We do not add these type definitions to SupportedUserTypes to avoid pulling <tuple> into our entire code
// base, which might potentially slow down compilation even further (although this was not tested or measured).
#include "Boolean.h"
#include "Void.h"

#include <cstdint>
#include <tuple>

namespace ChimeraTK {

  /**
   * Convenience type definition of a std::tuple with all possible UserTypes.
   *
   * This is a standard library replacement for ChimeraTK::userTypeMap.
   */
  using UserTypeTuple = std::tuple<ChimeraTK::Boolean, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t,
      uint64_t, float, double, std::string, ChimeraTK::Void>;

  /**
   * Like UserTypeVariUserTypeTupleant but without ChimeraTK::Void
   */
  using UserTypeTupleNoVoid = std::tuple<ChimeraTK::Boolean, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t,
      int64_t, uint64_t, float, double, std::string>;

  /**
   * Convenience type definition of a std::tuple with the given class template instantiated for all all possible
   * UserTypes.
   *
   * This is a standard library replacement for ChimeraTK::TemplateUserTypeMap.
   */
  template<template<typename> class TPL>
  using UserTypeTemplateTuple =
      std::tuple<TPL<ChimeraTK::Boolean>, TPL<int8_t>, TPL<uint8_t>, TPL<int16_t>, TPL<uint16_t>, TPL<int32_t>,
          TPL<uint32_t>, TPL<int64_t>, TPL<uint64_t>, TPL<float>, TPL<double>, TPL<std::string>, TPL<ChimeraTK::Void>>;

  /**
   * Like UserTypeTemplateTuple but without ChimeraTK::Void
   */
  template<template<typename> class TPL>
  using UserTypeTemplateTupleNoVoid =
      std::tuple<TPL<ChimeraTK::Boolean>, TPL<int8_t>, TPL<uint8_t>, TPL<int16_t>, TPL<uint16_t>, TPL<int32_t>,
          TPL<uint32_t>, TPL<int64_t>, TPL<uint64_t>, TPL<float>, TPL<double>, TPL<std::string>>;

  /**
   * Convenience type definition of a std::tuple with all possible RawTypes.
   */
  using RawTypeTuple = std::tuple<uint8_t, uint16_t, uint32_t, uint64_t>;

} // namespace ChimeraTK
