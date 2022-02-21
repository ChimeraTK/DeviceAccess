#pragma once

#include "NDRegisterAccessor.h"
#include "NumericAddressedBackend.h"

namespace ChimeraTK {
  template <typename UserType>
  class DummyInterruptTriggerAccessor : public NDRegisterAccessor<UserType> {
   public:
    DummyInterruptTriggerAccessor(boost::shared_ptr<DeviceBackend> backend,
        std::function<VersionNumber(void)> interruptTrigger, const RegisterPath& registerPathName)
    : NDRegisterAccessor<UserType>(registerPathName, {}), _backend(backend), _interruptTrigger(interruptTrigger) {
      NDRegisterAccessor<UserType>::buffer_2D.resize(1);
      NDRegisterAccessor<UserType>::buffer_2D[0].resize(1);
      NDRegisterAccessor<UserType>::buffer_2D[0][0] = numericToUserType<UserType>(0);
    }

    bool doWriteTransfer(ChimeraTK::VersionNumber) override {
      if(not _backend->isOpen()) {
        throw ChimeraTK::logic_error("Device is not opened.");
      }

      if (not _backend->isFunctional()) {
        throw ChimeraTK::runtime_error("Exception reported by another accessor.");
      }

      _interruptTrigger();

      return false;
    }

    void doReadTransferSynchronously() override {
    }

    void doPreRead(TransferType) override {
      if(not _backend->isOpen()) {
        throw ChimeraTK::logic_error("Device is not opened.");
      }

      if (not _backend->isFunctional()) {
        throw ChimeraTK::runtime_error("Exception reported by another accessor.");
      }
    }

    void doPostRead(TransferType, bool) override {
    }

    void doPreWrite(TransferType, VersionNumber) override {
      //
      if(not _backend->isOpen()) {
        throw ChimeraTK::logic_error("Device is not opened.");
      }

      if (not _backend->isFunctional()) {
        throw ChimeraTK::runtime_error("Exception reported by another accessor.");
      }
    }

    bool isReadOnly() const override { return false; }
    bool isReadable() const override { return true; }
    bool isWriteable() const override { return true; }

    void setExceptionBackend(boost::shared_ptr<DeviceBackend> exceptionBackend) override {
      this->_exceptionBackend = exceptionBackend;
    }

   protected:
    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override { return {}; }

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override { return {}; }

   private:
    boost::shared_ptr<DeviceBackend> _backend;
    std::function<VersionNumber(void)> _interruptTrigger;
  };

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(DummyInterruptTriggerAccessor);

} // namespace ChimeraTK

