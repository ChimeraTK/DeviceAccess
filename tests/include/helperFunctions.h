/*
 * helperFunctions.h
 *
 *  Created on: Aug 4, 2014
 *      Author: varghese
 */

#ifndef HELPERFUNCTIONS_H_
#define HELPERFUNCTIONS_H_

#include "MapFile.h"
#include "DMapFile.h"

void populateDummydRegisterInfoent(mtca4u::DMapFile::dRegisterInfo& dRegisterInfoent,
                              std::string dmapFileName,
                              std::string deviceName = "card",
                              std::string dev_file =
                                  "/dev/dummy_device_identifier",
                              std::string map_file_name =
                                  "/dev/dummy_map_file");
std::string appendNumberToName(std::string name, int cardNumber);
bool compareDRegisterInfoents(const mtca4u::DMapFile::dRegisterInfo& dRegisterInfoent1,
                         const mtca4u::DMapFile::dRegisterInfo& dRegisterInfoent2);

bool compareRegisterInfoents(const mtca4u::RegisterInfoMap::RegisterInfo& element1,
                        const mtca4u::RegisterInfoMap::RegisterInfo& element2);

std::string getCurrentWorkingDirectory();

#endif /* HELPERFUNCTIONS_H_ */
