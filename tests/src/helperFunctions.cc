#include "helperFunctions.h"

#include <sstream>

void populateDummyDeviceInfo(mtca4u::DeviceInfoMap::DeviceInfo& deviceInfo,
                              std::string dmapFileName, std::string deviceName,
                              std::string deviceFile, std::string mapFileName) {
  static int lineNumber = 1;
  if (deviceName == "card")
    deviceName = appendNumberToName(deviceName, lineNumber);
  if (deviceFile == "/dev/dummy_device_identifier")
    deviceFile = appendNumberToName(deviceName, lineNumber);
  if (mapFileName == "/dev/dummy_map_file")
    mapFileName = appendNumberToName(deviceName, lineNumber);

  deviceInfo.deviceName = deviceName;
  deviceInfo.deviceFile = deviceFile;
  deviceInfo.mapFileName = mapFileName;
  deviceInfo.dmapFileName = dmapFileName;
  deviceInfo.dmapFileLineNumber = ++lineNumber;
}

std::string appendNumberToName(std::string name, int suffixNumber) {
  std::stringstream deviceName;
  deviceName << name << suffixNumber;
  return (deviceName.str());
}

bool compareDeviceInfos(const mtca4u::DeviceInfoMap::DeviceInfo& deviceInfo1,
                         const mtca4u::DeviceInfoMap::DeviceInfo& deviceInfo2) {
  bool result =
      (deviceInfo1.deviceName == deviceInfo2.deviceName) &&
      (deviceInfo1.deviceFile == deviceInfo2.deviceFile) &&
      (deviceInfo1.mapFileName == deviceInfo2.mapFileName) &&
      (deviceInfo1.dmapFileName == deviceInfo2.dmapFileName) &&
      (deviceInfo1.dmapFileLineNumber == deviceInfo2.dmapFileLineNumber);
  return result;
}

bool compareRegisterInfoents(const mtca4u::RegisterInfoMap::RegisterInfo& element1,
                        const mtca4u::RegisterInfoMap::RegisterInfo& element2) {
  bool result = (element1._descriptionLineNumber == element2._descriptionLineNumber) &&
                (element1._addressOffset == element2._addressOffset) &&
                (element1._bar == element2._bar) &&
                (element1._elementCount == element2._elementCount) &&
                (element1._fractionalBits == element2._fractionalBits) &&
                (element1._name == element2._name) &&
                (element1._signedFlag == element2._signedFlag) &&
                (element1._size == element2._size) &&
                (element1._width == element2._width) &&
                (element1._module == element2._module);
  if (!result){
    std::cout << "Error in comparison: " << element1 << " : "<< element2 << std::endl;
  }
  return result;
}

std::string getCurrentWorkingDirectory() {
  char *currentWorkingDir = get_current_dir_name();
  if (!currentWorkingDir) {
    throw;
  }
  std::string dir(currentWorkingDir);
  free(currentWorkingDir);
  return dir;
}
