// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <variant>

namespace nlohmann {

  /********************************************************************************************************************/

  /**
   * Mapping of std::optional to JSON. nlohmann's template-specialisation lookup selects this specialisation for every
   * std::optional serialised in a translation unit including this header.
   */
  template<typename T>
  struct adl_serializer<std::optional<T>> {
    /** Called when an optional is written to JSON: writes the contained value, or null when it is empty. */
    static void to_json(json& j, const std::optional<T>& opt) {
      if(opt.has_value()) {
        j = *opt;
      }
      else {
        j = nullptr;
      }
    }

    /** Called when an optional is read from JSON: a null gives an empty optional, any other value the contained value. */
    static void from_json(const json& j, std::optional<T>& opt) {
      if(j.is_null()) {
        opt = std::nullopt;
      }
      else {
        opt = j.get<T>();
      }
    }
  };

  /********************************************************************************************************************/

  /**
   * Mapping of std::variant to JSON. nlohmann's template-specialisation lookup selects this specialisation for every
   * std::variant serialised in a translation unit including this header.
   */
  template<typename... Ts>
  struct adl_serializer<std::variant<Ts...>> {
    /** Called when a variant is written to JSON: writes the active alternative. A variant always holds a value. */
    static void to_json(json& j, const std::variant<Ts...>& var);

    /** Called when a variant is read from JSON: takes the first alternative that parses, a null and a value no
     * alternative accepts are rejected. */
    static void from_json(const json& j, std::variant<Ts...>& var);
  };

  /********************************************************************************************************************/

  template<typename... Ts>
  void adl_serializer<std::variant<Ts...>>::to_json(json& j, const std::variant<Ts...>& var) {
    std::visit([&j](const auto& value) { j = value; }, var);
  }

  /********************************************************************************************************************/

  template<typename... Ts>
  void adl_serializer<std::variant<Ts...>>::from_json(const json& j, std::variant<Ts...>& var) {
    bool parsed = false;
    // Try the alternatives in declaration order and keep the first one that parses, so the mapping is independent of
    // which alternatives happen to be ambiguous.
    auto tryParse = [&](const auto& value) {
      using T = std::decay_t<decltype(value)>;
      if(parsed) {
        return;
      }
      try {
        // Skip a null explicitly: nlohmann's from_json for POD types accepts null and leaves the value default, so it
        // would silently select the first alternative instead of reporting the malformed input. A variant always holds
        // a value, so no alternative reads a null.
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

  /********************************************************************************************************************/

} // namespace nlohmann
