// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NDRegisterAccessor.h"
#include "NumericAddressedBackend.h"

#include <utility>

namespace ChimeraTK {
  /**
   * @brief The DummyInterruptTriggerAccessor class
   *
   * Pseudo-accessor for the hidden DUMMY_INTERRUPT_X_Y register that can be used to trigger
   * the interrupt Y on interrupt controller X. This will do nothing but call DummyBackendBase::triggerInterrupt()
   * when written to. For backwards compatibility it can also be read from, but there is only a constant value
   * available.
   */
  template<typename UserType>
  class DummyInterruptTriggerAccessor : public NDRegisterAccessor<UserType> {
   public:
    DummyInterruptTriggerAccessor(boost::shared_ptr<DeviceBackend> backend,
        std::function<VersionNumber(void)> interruptTrigger, const RegisterPath& registerPathName,
        size_t numberOfElements = 1, size_t elementsOffset = 0, const AccessModeFlags& flags = {})
    : NDRegisterAccessor<UserType>(registerPathName, {}), _backend(std::move(backend)),
      _interruptTrigger(std::move(interruptTrigger)) {
      if(numberOfElements > 1) {
        throw ChimeraTK::logic_error("DUMMY_INTERRUPT accessor register can have at most one element");
      }

      if(elementsOffset != 0) {
        throw ChimeraTK::logic_error("DUMMY_INTERRUPT accessor register cannot have any offset");
      }

      flags.checkForUnknownFlags({});

      NDRegisterAccessor<UserType>::buffer_2D.resize(1);
      NDRegisterAccessor<UserType>::buffer_2D[0].resize(1);
      NDRegisterAccessor<UserType>::buffer_2D[0][0] = numericToUserType<UserType>(1);
    }

    bool doWriteTransfer(ChimeraTK::VersionNumber) override {
      _interruptTrigger();

      return false;
    }

    void doReadTransferSynchronously() override {}

    void doPreRead(TransferType) override {
      if(not _backend->isOpen()) {
        throw ChimeraTK::logic_error("Device is not opened.");
      }

      if(not _backend->isFunctional()) {
        throw ChimeraTK::runtime_error("Exception reported by another accessor.");
      }
    }

    void doPostRead(TransferType, bool hasNewData) override {
      if(not hasNewData) return;
      NDRegisterAccessor<UserType>::buffer_2D[0][0] = numericToUserType<UserType>(1);
      TransferElement::_versionNumber = {};
    }

    void doPreWrite(TransferType, VersionNumber) override {
      //
      if(not _backend->isOpen()) {
        throw ChimeraTK::logic_error("Device is not opened.");
      }

      if(not _backend->isFunctional()) {
        throw ChimeraTK::runtime_error("Exception reported by another accessor.");
      }
    }

    [[nodiscard]] bool isReadOnly() const override { return false; }
    [[nodiscard]] bool isReadable() const override { return true; }
    [[nodiscard]] bool isWriteable() const override { return true; }

   protected:
    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
      return {boost::enable_shared_from_this<TransferElement>::shared_from_this()};
    }

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override { return {}; }

   private:
    boost::shared_ptr<DeviceBackend> _backend;
    std::function<VersionNumber(void)> _interruptTrigger;
  };

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(DummyInterruptTriggerAccessor);

} // namespace ChimeraTK
