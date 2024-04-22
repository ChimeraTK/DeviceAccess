// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <vector>

namespace ChimeraTK {

  class DeviceBackend;
  class Device;
  class TransferGroup;

  template<typename UserType>
  class BufferingRegisterAccessor;

  template<typename UserType>
  class NDRegisterAccessor;

  template<typename UserType>
  class TwoDRegisterAccessor;

  template<typename UserType>
  class MultiplexedDataAccessor;

} // namespace ChimeraTK
