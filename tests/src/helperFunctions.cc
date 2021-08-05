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

bool compareRegisterInfoents(const ChimeraTK::RegisterInfoMap::RegisterInfo& element1,
    const ChimeraTK::RegisterInfoMap::RegisterInfo& element2) {
  bool result = (element1.address == element2.address) && (element1.bar == element2.bar) &&
      (element1.nElements == element2.nElements) && (element1.nFractionalBits == element2.nFractionalBits) &&
      (element1.name == element2.name) && (element1.signedFlag == element2.signedFlag) &&
      (element1.nBytes == element2.nBytes) && (element1.width == element2.width) &&
      (element1.module == element2.module) && (element1.registerAccess == element2.registerAccess) &&
      (element1.dataType == element2.dataType) &&
      (element1.getNumberOfDimensions() == element2.getNumberOfDimensions()) &&
      (element1.interruptCtrlNumber == element2.interruptCtrlNumber) &&
      (element1.interruptNumber == element2.interruptNumber);
  if(!result) {
    std::cout << "Error in comparison. Register 1: " << std::endl
              << element1 << std::endl
              << "Register 2:" << std::endl
              << element2 << std::endl;
  }
  return result;
}
