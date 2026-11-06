// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <variant>

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

  template<typename... Ts>
  struct adl_serializer<std::variant<Ts...>> {
    static void to_json(json& j, const std::variant<Ts...>& var) {
      std::visit([&j](const auto& value) { j = value; }, var);
    }

    static void from_json(const json& j, std::variant<Ts...>& var) {
      bool parsed = false;
      // Try the alternatives in declaration order and keep the first one that parses, so the mapping is independent
      // of which alternatives happen to be ambiguous. A variant always holds a value, so no null is accepted here.
      auto tryParse = [&](const auto& value) {
        using T = std::decay_t<decltype(value)>;
        if(parsed) {
          return;
        }
        try {
          // Skip a null explicitly: nlohmann's from_json for POD types accepts null and leaves the value default, so
          // it would silently select the first alternative instead of reporting the malformed input.
          if(j.is_null()) {
            return;
          }
          var = j.get<T>();
          parsed = true;
        }
        catch(const json::exception&) {
        }
      };
      (tryParse(Ts{}), ...);
      if(!parsed) {
        throw json::type_error::create(302, "No alternative of the variant matches the JSON value.", &j);
      }
    }
  };

} // namespace nlohmann
