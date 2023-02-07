// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DeviceBackend.h"
#include "Exception.h"

#include <atomic>
#include <list>

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * DeviceBackendImpl implements some basic functionality which should be available for all backends. This is required
   * to allow proper decorator patterns which should not have this functionality in the decorator itself.
   */
  class DeviceBackendImpl : public DeviceBackend {
   public:
    bool isOpen() override { return _opened; }

    bool isConnected() final {
      std::cerr << "Removed function DeviceBackendImpl::isConnected() called." << std::endl;
      std::cerr << "Do not use. This function has no valid meaning." << std::endl;
      std::terminate();
    }

    MetadataCatalogue getMetadataCatalogue() const override { return {}; }

   protected:
    /** flag if device is opened */
    std::atomic<bool> _opened{false};
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
