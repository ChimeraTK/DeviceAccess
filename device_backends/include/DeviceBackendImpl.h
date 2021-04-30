#ifndef CHIMERA_TK_DEVICE_BACKEND_IMPL_H
#define CHIMERA_TK_DEVICE_BACKEND_IMPL_H

#include <list>
#include <atomic>

#include "DeviceBackend.h"
#include "Exception.h"

namespace ChimeraTK {

  /**
   *  DeviceBackendImpl implements some basic functionality which should be
   * available for all backends. This is required to allow proper decorator
   * patterns which should not have this functionality in the decorator itself.
   */
  class DeviceBackendImpl : public DeviceBackend {
   public:
    DeviceBackendImpl();
    virtual ~DeviceBackendImpl();

    virtual bool isOpen() { return _opened; }

    virtual bool isConnected() { return _connected; }

    virtual const RegisterCatalogue& getRegisterCatalogue() const { return _catalogue; }

   protected:
    /** the register catalogue containing describing the registers known by this
     * backend */
    RegisterCatalogue _catalogue;

    /** flag if device is opened */
    std::atomic<bool> _opened = {true};

    /** flag if device is connected. */
    bool _connected;

  };

} // namespace ChimeraTK

#endif /*CHIMERA_TK_DEVICE_BACKEND_IMPL_H*/
