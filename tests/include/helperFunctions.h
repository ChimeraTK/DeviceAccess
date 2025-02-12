// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DeviceInfoMap.h"

void populateDummyDeviceInfo(ChimeraTK::DeviceInfoMap::DeviceInfo& deviceInfo, std::string dmapFileName,
    std::string deviceName = "card", std::string dev_file = "/dev/dummy_device_identifier",
    std::string map_file_name = "/dev/dummy_map_file");

std::string appendNumberToName(std::string name, int cardNumber);

bool compareDeviceInfos(
    const ChimeraTK::DeviceInfoMap::DeviceInfo& deviceInfo1, const ChimeraTK::DeviceInfoMap::DeviceInfo& deviceInfo2);
