// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "helperFunctions.h"

#include <sstream>

void populateDummyDeviceInfo(ChimeraTK::DeviceInfoMap::DeviceInfo& deviceInfo, std::string dmapFileName,
    std::string deviceName, std::string deviceFile, std::string mapFileName) {
  static int lineNumber = 1;
  if(deviceName == "card") deviceName = appendNumberToName(deviceName, lineNumber);
  if(deviceFile == "/dev/dummy_device_identifier") deviceFile = appendNumberToName(deviceName, lineNumber);
  if(mapFileName == "/dev/dummy_map_file") mapFileName = appendNumberToName(deviceName, lineNumber);

  deviceInfo.deviceName = deviceName;
  deviceInfo.uri = deviceFile;
  deviceInfo.mapFileName = mapFileName;
  deviceInfo.dmapFileName = dmapFileName;
  deviceInfo.dmapFileLineNumber = ++lineNumber;
}

std::string appendNumberToName(std::string name, int suffixNumber) {
  std::stringstream deviceName;
  deviceName << name << suffixNumber;
  return (deviceName.str());
}

bool compareDeviceInfos(
    const ChimeraTK::DeviceInfoMap::DeviceInfo& deviceInfo1, const ChimeraTK::DeviceInfoMap::DeviceInfo& deviceInfo2) {
  bool result = (deviceInfo1.deviceName == deviceInfo2.deviceName) && (deviceInfo1.uri == deviceInfo2.uri) &&
      (deviceInfo1.mapFileName == deviceInfo2.mapFileName) && (deviceInfo1.dmapFileName == deviceInfo2.dmapFileName) &&
      (deviceInfo1.dmapFileLineNumber == deviceInfo2.dmapFileLineNumber);
  return result;
}
