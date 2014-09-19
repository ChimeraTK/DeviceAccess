/*
 * helperFunctions.h
 *
 *  Created on: Aug 4, 2014
 *      Author: varghese
 */

#ifndef HELPERFUNCTIONS_H_
#define HELPERFUNCTIONS_H_

#include "mapFile.h"
#include "dmapFile.h"

void populateDummydMapElement(mtca4u::dmapFile::dmapElem& dMapElement,
                              std::string dmapFileName,
                              std::string deviceName = "card",
                              std::string dev_file =
                                  "/dev/dummy_device_identifier",
                              std::string map_file_name =
                                  "/dev/dummy_map_file");
std::string appendNumberToName(std::string name, int cardNumber);
bool compareDMapElements(const mtca4u::dmapFile::dmapElem& dMapElement1,
                         const mtca4u::dmapFile::dmapElem& dMapElement2);

bool compareMapElements(const mtca4u::mapFile::mapElem& element1,
                        const mtca4u::mapFile::mapElem& element2);

std::string getCurrentWorkingDirectory();

void populateDummydMapElement(mtca4u::dmapFile::dmapElem& dMapElement,
                              std::string dmapFileName, std::string deviceName,
                              std::string dev_file, std::string map_file_name) {
  static int lineNumber = 1;
  if (deviceName == "card")
    deviceName = appendNumberToName(deviceName, lineNumber);
  if (dev_file == "/dev/dummy_device_identifier")
    dev_file = appendNumberToName(deviceName, lineNumber);
  if (map_file_name == "/dev/dummy_map_file")
    map_file_name = appendNumberToName(deviceName, lineNumber);

  dMapElement.dev_name = deviceName;
  dMapElement.dev_file = dev_file;
  dMapElement.map_file_name = map_file_name;
  dMapElement.dmap_file_name = dmapFileName;
  dMapElement.dmap_file_line_nr = ++lineNumber;
}

std::string appendNumberToName(std::string name, int suffixNumber) {
  std::stringstream deviceName;
  deviceName << name << suffixNumber;
  return (deviceName.str());
}

bool compareDMapElements(const mtca4u::dmapFile::dmapElem& dMapElement1,
                         const mtca4u::dmapFile::dmapElem& dMapElement2) {
  bool result =
      (dMapElement1.dev_name == dMapElement2.dev_name) &&
      (dMapElement1.dev_file == dMapElement2.dev_file) &&
      (dMapElement1.map_file_name == dMapElement2.map_file_name) &&
      (dMapElement1.dmap_file_name == dMapElement2.dmap_file_name) &&
      (dMapElement1.dmap_file_line_nr == dMapElement2.dmap_file_line_nr);
  return result;
}

bool compareMapElements(const mtca4u::mapFile::mapElem& element1,
                        const mtca4u::mapFile::mapElem& element2) {
  bool result = (element1.line_nr == element2.line_nr) &&
                (element1.reg_address == element2.reg_address) &&
                (element1.reg_bar == element2.reg_bar) &&
                (element1.reg_elem_nr == element2.reg_elem_nr) &&
                (element1.reg_frac_bits == element2.reg_frac_bits) &&
                (element1.reg_name == element2.reg_name) &&
                (element1.reg_signed == element2.reg_signed) &&
                (element1.reg_size == element2.reg_size) &&
                (element1.reg_width == element2.reg_width);
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


#endif /* HELPERFUNCTIONS_H_ */
