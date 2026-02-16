// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NDRegisterAccessor.h"
#include "NDRegisterAccessorDecorator.h"
#include "NumericAddressedBackend.h"
#include "RegisterInfo.h"
#include "TransferElement.h"

#include <string>

namespace ChimeraTK {

  template<typename UserType>
  class DoubleBufferAccessorDecorator : public NDRegisterAccessorDecorator<UserType> {
   public:
    using ChimeraTK::NDRegisterAccessorDecorator<UserType>::buffer_2D;
    using ChimeraTK::NDRegisterAccessorDecorator<UserType>::_target;
    DoubleBufferAccessorDecorator(const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>>& target,
        NumericAddressedRegisterInfo::DoubleBufferInfo doubleBufferConfig,
        const boost::shared_ptr<DeviceBackend>& backend, std::shared_ptr<detail::CountedRecursiveMutex> mutex);

    void doPreRead(TransferType type) override;

    void doReadTransferSynchronously() override;

    void doPostRead(TransferType type, bool hasNewData) override;

    [[nodiscard]] bool isWriteable() const override { return false; }

    void doPreWrite(TransferType, VersionNumber) override {
      throw ChimeraTK::logic_error("NumericAddressBackend DoubleBufferPlugin: Writing is not allowed atm.");
    }

    void doPostWrite(TransferType, VersionNumber) override {
      // do not throw here again
    }
    // below functions are needed for TransferGroup to work
    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override;

    // porbably it's enough not override these methods
    // std::list<boost::shared_ptr<TransferElement>> getInternalElements() override { return {}; }

    // void replaceTransferElement(boost::shared_ptr<ChimeraTK::TransferElement> /* newElement */) override {
    //  do nothing, we do not support merging of DoubleBufferAccessorDecorators
    //}
    [[nodiscard]] bool mayReplaceOther(const boost::shared_ptr<TransferElement const>& other) const override;

   private:
    NumericAddressedRegisterInfo::DoubleBufferInfo _doubleBufferInfo;
    boost::shared_ptr<DeviceBackend> _backend;
    std::shared_ptr<detail::CountedRecursiveMutex> _mutex;
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> _secondBufferReg;
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<uint32_t>> _enableDoubleBufferReg;
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<uint32_t>> _currentBufferNumberReg;
    uint32_t _currentBuffer{0};
  };
  // Factory for instantiation
  template<typename T>
  boost::shared_ptr<NDRegisterAccessor<T>> createDoubleBufferDecorator(
      const boost::shared_ptr<NDRegisterAccessor<T>>& target,
      NumericAddressedRegisterInfo::DoubleBufferInfo doubleBufferConfig,
      const boost::shared_ptr<DeviceBackend>& backend, std::shared_ptr<detail::CountedRecursiveMutex> mutex) {
    return boost::make_shared<DoubleBufferAccessorDecorator<T>>(target, doubleBufferConfig, backend, mutex);
  }

  template<typename T>
  struct CreateDoubleBufferDecoratorInstancer {
    CreateDoubleBufferDecoratorInstancer() : ptr(&createDoubleBufferDecorator<T>) {}
    boost::shared_ptr<NDRegisterAccessor<T>> (*ptr)(const boost::shared_ptr<NDRegisterAccessor<T>>&,
        NumericAddressedRegisterInfo::DoubleBufferInfo, const boost::shared_ptr<DeviceBackend>&,
        std::shared_ptr<detail::CountedRecursiveMutex>);
  };

  // Global registrar to force instantiation
  extern ChimeraTK::TemplateUserTypeMap<CreateDoubleBufferDecoratorInstancer> _createDoubleBufferDecorator_instancer;

} // namespace ChimeraTK
