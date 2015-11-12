#include "helperFunctions.h"

#include <sstream>

void populateDummydRegisterInfoent(mtca4u::DeviceInfoMap::DeviceInfo& dRegisterInfoent,
                              std::string dmapFileName, std::string deviceName,
                              std::string dev_file, std::string map_file_name) {
  static int lineNumber = 1;
  if (deviceName == "card")
    deviceName = appendNumberToName(deviceName, lineNumber);
  if (dev_file == "/dev/dummy_device_identifier")
    dev_file = appendNumberToName(deviceName, lineNumber);
  if (map_file_name == "/dev/dummy_map_file")
    map_file_name = appendNumberToName(deviceName, lineNumber);

  dRegisterInfoent.dev_name = deviceName;
  dRegisterInfoent.dev_file = dev_file;
  dRegisterInfoent.map_file_name = map_file_name;
  dRegisterInfoent.dmap_file_name = dmapFileName;
  dRegisterInfoent.dmap_file_line_nr = ++lineNumber;
}

std::string appendNumberToName(std::string name, int suffixNumber) {
  std::stringstream deviceName;
  deviceName << name << suffixNumber;
  return (deviceName.str());
}

bool compareDRegisterInfoents(const mtca4u::DeviceInfoMap::DeviceInfo& dRegisterInfoent1,
                         const mtca4u::DeviceInfoMap::DeviceInfo& dRegisterInfoent2) {
  bool result =
      (dRegisterInfoent1.dev_name == dRegisterInfoent2.dev_name) &&
      (dRegisterInfoent1.dev_file == dRegisterInfoent2.dev_file) &&
      (dRegisterInfoent1.map_file_name == dRegisterInfoent2.map_file_name) &&
      (dRegisterInfoent1.dmap_file_name == dRegisterInfoent2.dmap_file_name) &&
      (dRegisterInfoent1.dmap_file_line_nr == dRegisterInfoent2.dmap_file_line_nr);
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
