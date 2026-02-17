// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

// Note: We do not add these type definitions to SupportedUserTypes to avoid pulling <variant> into our entire code
// base, which might potentially slow down compilation even further (although this was not tested or measured).
#include "Boolean.h"
#include "Void.h"

#include <cstdint>
#include <variant>

namespace ChimeraTK {

  /**
   * Convenience type definition of a std::variant with all possible UserTypes.
   *
   * In contrast to ChimeraTK::userTypeMap, only one value is stored in the UserTypeVariant, and there is some runtime
   * overhead to access the value (which UserType is stored in the variant is a runtime decision).
   */
  using UserTypeVariant = std::variant<ChimeraTK::Boolean, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t,
      int64_t, uint64_t, float, double, std::string, ChimeraTK::Void>;

  /**
   * Like UserTypeVariant but without ChimeraTK::Void
   */
  using UserTypeVariantNoVoid = std::variant<ChimeraTK::Boolean, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t,
      int64_t, uint64_t, float, double, std::string>;

  /**
   * Convenience type definition of a std::variant with the given class template instantiated for all all possible
   * UserTypes.
   *
   * In contrast to ChimeraTK::TemplateUserTypeMap, only one value is stored in the UserTypeTemplateVariant, and there
   * is some runtime overhead to access the value (which UserType is stored in the variant is a runtime decision).
   */
  template<template<typename> class TPL>
  using UserTypeTemplateVariant =
      std::variant<TPL<ChimeraTK::Boolean>, TPL<int8_t>, TPL<uint8_t>, TPL<int16_t>, TPL<uint16_t>, TPL<int32_t>,
          TPL<uint32_t>, TPL<int64_t>, TPL<uint64_t>, TPL<float>, TPL<double>, TPL<std::string>, TPL<ChimeraTK::Void>>;

  /**
   * Like UserTypeTemplateVariant but without ChimeraTK::Void
   */
  template<template<typename> class TPL>
  using UserTypeTemplateVariantNoVoid =
      std::variant<TPL<ChimeraTK::Boolean>, TPL<int8_t>, TPL<uint8_t>, TPL<int16_t>, TPL<uint16_t>, TPL<int32_t>,
          TPL<uint32_t>, TPL<int64_t>, TPL<uint64_t>, TPL<float>, TPL<double>, TPL<std::string>>;

} // namespace ChimeraTK
