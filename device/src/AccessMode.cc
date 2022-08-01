// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AccessMode.h"

#include "Exception.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  AccessModeFlags::AccessModeFlags(std::set<AccessMode> flags) : _flags(std::move(flags)) {}

  /********************************************************************************************************************/

  AccessModeFlags::AccessModeFlags(const std::initializer_list<AccessMode>& flags) : _flags(flags) {}

  /********************************************************************************************************************/

  bool AccessModeFlags::has(AccessMode flag) const {
    return (_flags.count(flag) != 0);
  }

  /********************************************************************************************************************/

  bool AccessModeFlags::empty() const {
    return _flags.empty();
  }

  /********************************************************************************************************************/

  void AccessModeFlags::checkForUnknownFlags(const std::set<AccessMode>& knownFlags) const {
    for(auto flag : _flags) {
      if(knownFlags.count(flag) == 0) {
        throw ChimeraTK::logic_error("Access mode flag '" + getString(flag) + "' is not known by this backend.");
      }
    }
  }

  /********************************************************************************************************************/

  bool AccessModeFlags::operator==(const AccessModeFlags& other) const {
    // fortunately the std::set already has a comparison operator which does exacty what we want
    return _flags == other._flags;
  }

  /********************************************************************************************************************/

  bool AccessModeFlags::operator<(const AccessModeFlags& other) const {
    // fortunately the std::set already has a comparison operator which does exacty what we want
    return _flags < other._flags;
  }

  /********************************************************************************************************************/

  void AccessModeFlags::remove(AccessMode flag) {
    _flags.erase(flag);
  }

  /********************************************************************************************************************/

  void AccessModeFlags::add(AccessMode flag) {
    _flags.insert(flag);
  }

  /********************************************************************************************************************/

  std::string AccessModeFlags::serialize() const {
    std::string list{};
    for(const auto& f : _flags) {
      list += getString(f) + ",";
    }
    if(!list.empty()) {
      list.pop_back(); // remove trailing ','
    }
    return list;
  }

  /********************************************************************************************************************/

  const std::string& AccessModeFlags::getString(const AccessMode flag) {
    return getStringMap().at(flag);
  }

  /********************************************************************************************************************/

  AccessModeFlags AccessModeFlags::deserialize(const std::string& listOfflags) {
    std::vector<std::string> names = split(listOfflags);
    std::set<AccessMode> flagList;
    for(const auto& flagName : names) {
      flagList.insert(getAccessMode(flagName));
    }
    return AccessModeFlags{flagList};
  }

  /********************************************************************************************************************/

  const std::map<AccessMode, std::string>& AccessModeFlags::getStringMap() {
    static std::map<AccessMode, std::string> m = {
        {AccessMode::raw, "raw"}, {AccessMode::wait_for_new_data, "wait_for_new_data"}};
    return m;
  }

  /********************************************************************************************************************/

  AccessMode AccessModeFlags::getAccessMode(const std::string& flagName) {
    static std::map<std::string, AccessMode> reverse_m;
    for(const auto& m : getStringMap()) {
      reverse_m[m.second] = m.first;
    }
    try {
      return reverse_m.at(flagName);
    }
    catch(std::out_of_range& e) {
      throw ChimeraTK::logic_error("Unknown flag string: " + flagName);
    }
  }

  /********************************************************************************************************************/

  std::vector<std::string> AccessModeFlags::split(const std::string& s) {
    std::vector<std::string> list;
    std::string tmp;
    char delimiter = ',';

    std::istringstream stream(s);
    while(getline(stream, tmp, delimiter)) {
      list.push_back(tmp);
    }
    return list;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
