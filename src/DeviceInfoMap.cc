// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DeviceInfoMap.h"

#include "Exception.h"
#include "predicates.h"

#include <algorithm>
#include <utility>

namespace ChimeraTK {

  DeviceInfoMap::DeviceInfoMap(std::string fileName) : _dmapFileName(std::move(fileName)) {}

  size_t DeviceInfoMap::getSize() {
    return _deviceInfoElements.size();
  }

  std::ostream& operator<<(std::ostream& os, const DeviceInfoMap& deviceInfoMap) {
    size_t size;
    os << "=======================================" << std::endl;
    os << "MAP FILE NAME: " << deviceInfoMap._dmapFileName << std::endl;
    os << "---------------------------------------" << std::endl;
    for(size = 0; size < deviceInfoMap._deviceInfoElements.size(); size++) {
      os << deviceInfoMap._deviceInfoElements[size] << std::endl;
    }
    os << "=======================================";
    return os;
  }

  void DeviceInfoMap::insert(const DeviceInfo& elem) {
    _deviceInfoElements.push_back(elem);
  }

  void DeviceInfoMap::getDeviceInfo(const std::string& deviceName, DeviceInfo& value) {
    std::vector<DeviceInfo>::iterator iter;
    iter = find_if(_deviceInfoElements.begin(), _deviceInfoElements.end(), findDevByName_pred(deviceName));
    if(iter == _deviceInfoElements.end()) {
      throw ChimeraTK::logic_error("Cannot find device \"" + deviceName + "\" in DMAP file:" + _dmapFileName);
    }
    value = *iter;
  }

  DeviceInfoMap::DeviceInfo::DeviceInfo() : dmapFileLineNumber(0) {}

  std::pair<std::string, std::string> DeviceInfoMap::DeviceInfo::getDeviceFileAndMapFileName() const {
    return {uri, mapFileName};
  }

  std::ostream& operator<<(std::ostream& os, const DeviceInfoMap::DeviceInfo& deviceInfo) {
    os << "(" << deviceInfo.dmapFileName << ") NAME: " << deviceInfo.deviceName << " DEV : " << deviceInfo.uri
       << " MAP : " << deviceInfo.mapFileName;
    return os;
  }

  // fixme: why is level not used?
  bool DeviceInfoMap::check(DeviceInfoMap::ErrorList& err, DeviceInfoMap::ErrorList::ErrorElem::TYPE /*level*/) {
    std::vector<DeviceInfoMap::DeviceInfo> dmaps = _deviceInfoElements;
    std::vector<DeviceInfoMap::DeviceInfo>::iterator iter_p, iter_n;
    bool ret = true;

    err.clear();
    if(dmaps.size() < 2) {
      return true;
    }

    std::sort(dmaps.begin(), dmaps.end(), copmaredRegisterInfosByName2_functor());
    iter_p = dmaps.begin();
    iter_n = iter_p + 1;
    while(true) {
      if((*iter_p).deviceName == (*iter_n).deviceName) {
        if((*iter_p).uri != (*iter_n).uri || (*iter_p).mapFileName != (*iter_n).mapFileName) {
          err.insert(ErrorList::ErrorElem(
              ErrorList::ErrorElem::ERROR, ErrorList::ErrorElem::NONUNIQUE_DEVICE_NAME, (*iter_p), (*iter_n)));
          ret = false;
        }
      }
      iter_p = iter_n;
      iter_n = ++iter_n;
      if(iter_n == dmaps.end()) {
        break;
      }
    }
    return ret;
  }

  std::ostream& operator<<(std::ostream& os, const DeviceInfoMap::ErrorList::ErrorElem::TYPE& me) {
    switch(me) {
      case DeviceInfoMap::ErrorList::ErrorElem::ERROR:
        os << "ERROR";
        break;
      case DeviceInfoMap::ErrorList::ErrorElem::WARNING:
        os << "WARNING";
        break;
      default:
        os << "UNKNOWN";
        break;
    }
    return os;
  }

  DeviceInfoMap::ErrorList::ErrorElem::ErrorElem(TYPE infoType, DMAP_FILE_ERR errorType,
      const DeviceInfoMap::DeviceInfo& device1, const DeviceInfoMap::DeviceInfo& device2) {
    _errorType = errorType;
    _errorDevice1 = device1;
    _errorDevice2 = device2;
    _type = infoType;
  }

  std::ostream& operator<<(std::ostream& os, const DeviceInfoMap::ErrorList::ErrorElem& me) {
    switch(me._errorType) {
      case DeviceInfoMap::ErrorList::ErrorElem::NONUNIQUE_DEVICE_NAME:
        os << me._type << ": Found two devices with the same name but different properties: \""
           << me._errorDevice1.deviceName << "\" in file \"" << me._errorDevice1.dmapFileName << "\" in line "
           << me._errorDevice1.dmapFileLineNumber << " and \"" << me._errorDevice2.dmapFileName << "\" in line "
           << me._errorDevice2.dmapFileLineNumber;
        break;
    }
    return os;
  }

  void DeviceInfoMap::ErrorList::clear() {
    _errors.clear();
  }

  void DeviceInfoMap::ErrorList::insert(const ErrorElem& elem) {
    _errors.push_back(elem);
  }

  std::ostream& operator<<(std::ostream& os, const DeviceInfoMap::ErrorList& me) {
    std::list<DeviceInfoMap::ErrorList::ErrorElem>::const_iterator iter;
    for(iter = me._errors.begin(); iter != me._errors.end(); ++iter) {
      os << (*iter) << std::endl;
    }
    return os;
  }

  DeviceInfoMap::iterator DeviceInfoMap::begin() {
    return _deviceInfoElements.begin();
  }

  DeviceInfoMap::iterator DeviceInfoMap::end() {
    return _deviceInfoElements.end();
  }

  void DeviceInfoMap::addPluginLibrary(const std::string& soFile) {
    _pluginLibraries.push_back(soFile);
  }

  std::vector<std::string> DeviceInfoMap::getPluginLibraries() {
    return _pluginLibraries;
  }

} // namespace ChimeraTK
