#pragma once

#include "DeviceBackend.h"
#include "Exception.h"

#include <list>
#include <atomic>

namespace ChimeraTK {

  /**
   *  DeviceBackendImpl implements some basic functionality which should be
   * available for all backends. This is required to allow proper decorator
   * patterns which should not have this functionality in the decorator itself.
   */
  class DeviceBackendImpl : public DeviceBackend {
   public:
    bool isOpen() override { return _opened; }

    bool isConnected() override { return _connected; }

   protected:

    /** flag if device is opened */
    std::atomic<bool> _opened{false};

    /** flag if device is connected. */
    bool _connected{false};
  };

} // namespace ChimeraTK
