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

    virtual void read(uint8_t /*bar*/, uint32_t /*address*/, int32_t* /*data*/, size_t /*sizeInBytes*/) {
      throw ChimeraTK::logic_error("The depcrecated DeviceBackend::read() "
                                   "function is not implemented by this backend. "
                                   "Use the Device frontend instead!");
    }

    virtual void write(uint8_t /*bar*/, uint32_t /*address*/, int32_t const* /*data*/, size_t /*sizeInBytes*/) {
      throw ChimeraTK::logic_error("The depcrecated DeviceBackend::write() "
                                   "function is not implemented by this backend. "
                                   "Use the Device frontend instead!");
    }

    virtual void read(const std::string&, const std::string&, int32_t*, size_t = 0, uint32_t = 0) {
      throw ChimeraTK::logic_error("The depcrecated DeviceBackend::read() "
                                   "function is not implemented by this backend. "
                                   "Use the Device frontend instead!");
    }

    virtual void write(const std::string&, const std::string&, int32_t const*, size_t = 0, uint32_t = 0) {
      throw ChimeraTK::logic_error("The depcrecated DeviceBackend::write() "
                                   "function is not implemented by this backend. "
                                   "Use the Device frontend instead!");
    }

    virtual boost::shared_ptr<const RegisterInfoMap> getRegisterMap() const {
      // implementing this read function is not mandatory, so we throw a
      // not-implemented exception by default
      throw ChimeraTK::logic_error("Obtaining a register map is not supported by this backend.");
    }

   protected:
    /** the register catalogue containing describing the registers known by this
     * backend */
    RegisterCatalogue _catalogue;

    /** flag if device is opened */
    std::atomic<bool> _opened;

    /** flag if device is connected. */
    bool _connected;
  };

} // namespace ChimeraTK

#endif /*CHIMERA_TK_DEVICE_BACKEND_IMPL_H*/
