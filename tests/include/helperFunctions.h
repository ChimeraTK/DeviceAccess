/*
 * helperFunctions.h
 *
 *  Created on: Aug 4, 2014
 *      Author: varghese
 */

#pragma once

#include "DeviceInfoMap.h"
#include "NumericAddressedRegisterCatalogue.h"

void populateDummyDeviceInfo(ChimeraTK::DeviceInfoMap::DeviceInfo& deviceInfo, std::string dmapFileName,
    std::string deviceName = "card", std::string dev_file = "/dev/dummy_device_identifier",
    std::string map_file_name = "/dev/dummy_map_file");

std::string appendNumberToName(std::string name, int cardNumber);

bool compareDeviceInfos(const ChimeraTK::DeviceInfoMap::DeviceInfo& deviceInfo1,
    const ChimeraTK::DeviceInfoMap::DeviceInfo& deviceInfo2);
