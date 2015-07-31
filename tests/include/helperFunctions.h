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

void populateDummydMapElement(mtca4u::DMapFile::dmapElem& dMapElement,
                              std::string dmapFileName,
                              std::string deviceName = "card",
                              std::string dev_file =
                                  "/dev/dummy_device_identifier",
                              std::string map_file_name =
                                  "/dev/dummy_map_file");
std::string appendNumberToName(std::string name, int cardNumber);
bool compareDMapElements(const mtca4u::DMapFile::dmapElem& dMapElement1,
                         const mtca4u::DMapFile::dmapElem& dMapElement2);

bool compareMapElements(const mtca4u::mapFile::mapElem& element1,
                        const mtca4u::mapFile::mapElem& element2);

std::string getCurrentWorkingDirectory();

#endif /* HELPERFUNCTIONS_H_ */
