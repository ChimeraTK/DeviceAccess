/*
 * helperFunctions.h
 *
 *  Created on: Aug 4, 2014
 *      Author: varghese
 */

#ifndef HELPERFUNCTIONS_H_
#define HELPERFUNCTIONS_H_

#include "DeviceInfoMap.h"
#include "RegisterInfoMap.h"

void populateDummyDeviceInfo(
    ChimeraTK::DeviceInfoMap::DeviceInfo &deviceInfo, std::string dmapFileName,
    std::string deviceName = "card",
    std::string dev_file = "/dev/dummy_device_identifier",
    std::string map_file_name = "/dev/dummy_map_file");
std::string appendNumberToName(std::string name, int cardNumber);
bool compareDeviceInfos(
    const ChimeraTK::DeviceInfoMap::DeviceInfo &deviceInfo1,
    const ChimeraTK::DeviceInfoMap::DeviceInfo &deviceInfo2);

bool compareRegisterInfoents(
    const ChimeraTK::RegisterInfoMap::RegisterInfo &element1,
    const ChimeraTK::RegisterInfoMap::RegisterInfo &element2);

#endif /* HELPERFUNCTIONS_H_ */
