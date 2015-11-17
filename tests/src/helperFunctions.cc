#include "helperFunctions.h"

#include <sstream>

void populateDummyDeviceInfo(mtca4u::DeviceInfoMap::DeviceInfo& deviceInfo,
                              std::string dmapFileName, std::string deviceName,
                              std::string _deviceFile, std::string _mapFileName) {
  static int lineNumber = 1;
  if (deviceName == "card")
    deviceName = appendNumberToName(deviceName, lineNumber);
  if (_deviceFile == "/dev/dummy_device_identifier")
    _deviceFile = appendNumberToName(deviceName, lineNumber);
  if (_mapFileName == "/dev/dummy_map_file")
    _mapFileName = appendNumberToName(deviceName, lineNumber);

  deviceInfo._deviceName = deviceName;
  deviceInfo._deviceFile = _deviceFile;
  deviceInfo._mapFileName = _mapFileName;
  deviceInfo._dmapFileName = dmapFileName;
  deviceInfo._dmapFileLineNumber = ++lineNumber;
}

std::string appendNumberToName(std::string name, int suffixNumber) {
  std::stringstream deviceName;
  deviceName << name << suffixNumber;
  return (deviceName.str());
}

bool compareDeviceInfos(const mtca4u::DeviceInfoMap::DeviceInfo& deviceInfo1,
                         const mtca4u::DeviceInfoMap::DeviceInfo& deviceInfo2) {
  bool result =
      (deviceInfo1._deviceName == deviceInfo2._deviceName) &&
      (deviceInfo1._deviceFile == deviceInfo2._deviceFile) &&
      (deviceInfo1._mapFileName == deviceInfo2._mapFileName) &&
      (deviceInfo1._dmapFileName == deviceInfo2._dmapFileName) &&
      (deviceInfo1._dmapFileLineNumber == deviceInfo2._dmapFileLineNumber);
  return result;
}

bool compareRegisterInfoents(const mtca4u::RegisterInfoMap::RegisterInfo& element1,
                        const mtca4u::RegisterInfoMap::RegisterInfo& element2) {
  bool result = (element1.line_nr == element2.line_nr) &&
                (element1.reg_address == element2.reg_address) &&
                (element1.reg_bar == element2.reg_bar) &&
                (element1.reg_elem_nr == element2.reg_elem_nr) &&
                (element1.reg_frac_bits == element2.reg_frac_bits) &&
                (element1.reg_name == element2.reg_name) &&
                (element1.reg_signed == element2.reg_signed) &&
                (element1.reg_size == element2.reg_size) &&
                (element1.reg_width == element2.reg_width) &&
                (element1.reg_module == element2.reg_module);
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
