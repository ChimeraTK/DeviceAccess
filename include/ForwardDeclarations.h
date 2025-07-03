// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "SupportedUserTypes.h"
namespace ChimeraTK {

  class DeviceBackend;
  class Device;
  class TransferGroup;

  template<user_type UserType>
  class NDRegisterAccessor;

  template<user_type UserType>
  class TwoDRegisterAccessor;

} // namespace ChimeraTK
