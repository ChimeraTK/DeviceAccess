// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Exception.h"
#include "LNMAccessorPlugin.h"
#include "LNMBackendRegisterInfo.h"

#include <regex>

namespace ChimeraTK::LNMBackend {
  TagModifierPlugin::TagModifierPlugin(
      const LNMBackendRegisterInfo& info, size_t pluginIndex, const std::map<std::string, std::string>& parameters)
  : AccessorPlugin<TagModifierPlugin>(info, pluginIndex) {
    const static std::regex re(R"(\s*,\s*)");

    if(parameters.contains("add")) {
      auto str = parameters.at("add");
      _tagsToAdd.insert(std::sregex_token_iterator(str.begin(), str.end(), re, -1), std::sregex_token_iterator());
    }

    if(parameters.contains("remove")) {
      auto str = parameters.at("remove");
      _tagsToRemove.insert(std::sregex_token_iterator(str.begin(), str.end(), re, -1), std::sregex_token_iterator());
    }

    if(parameters.contains("set")) {
      auto str = parameters.at("set");
      _tagsToSet.insert(std::sregex_token_iterator(str.begin(), str.end(), re, -1), std::sregex_token_iterator());
    }

    if(_tagsToAdd.empty() && _tagsToRemove.empty() && _tagsToSet.empty()) {
      throw ChimeraTK::logic_error(R"(TagModifierPlugin needs at least one of "add","remove" or "set")");
    }

    if(!_tagsToSet.empty() && !(_tagsToRemove.empty() || _tagsToAdd.empty())) {
      throw ChimeraTK::logic_error(R"(TagModifierPlugin: "set" is mutual exclusive with "add" and "remove")");
    }
  }

  /********************************************************************************************************************/

  void TagModifierPlugin::doRegisterInfoUpdate() {
    if(!_tagsToSet.empty()) {
      _info.tags = _tagsToSet;
    }
    else {
      _info.tags.insert(_tagsToAdd.begin(), _tagsToAdd.end());
      std::ranges::set_difference(_info.tags, _tagsToRemove, std::inserter(_tagsToSet, _tagsToSet.end()));
      _info.tags = _tagsToSet;
    }
  }
} // namespace ChimeraTK::LNMBackend
