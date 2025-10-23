// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <nlohmann/json.hpp>

#include <optional>

namespace nlohmann {

  template<typename T>
  struct adl_serializer<std::optional<T>> {
    static void to_json(json& j, const std::optional<T>& opt) {
      if(opt.has_value()) {
        j = *opt;
      }
      else {
        j = nullptr;
      }
    }

    static void from_json(const json& j, std::optional<T>& opt) {
      if(j.is_null()) {
        opt = std::nullopt;
      }
      else {
        opt = j.get<T>();
      }
    }
  };

} // namespace nlohmann
