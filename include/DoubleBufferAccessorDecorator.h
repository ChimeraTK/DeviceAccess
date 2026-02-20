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
  class DoubleBufferAccessorDecorator : public NDRegisterAccessor<UserType> {
   public:
    DoubleBufferAccessorDecorator(NumericAddressedRegisterInfo::DoubleBufferInfo doubleBufferConfig,
        const boost::shared_ptr<DeviceBackend>& backend, std::shared_ptr<detail::CountedRecursiveMutex> mutex,
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

    void doPreRead(TransferType type) override;

    void doReadTransferSynchronously() override;

    void doPostRead(TransferType type, bool hasNewData) override;

    [[nodiscard]] bool isWriteable() const override { return false; }
    [[nodiscard]] bool isReadOnly() const override;

    [[nodiscard]] bool isReadable() const override;

    //[[nodiscard]] bool isWriteable() const override;

    bool doWriteTransfer(ChimeraTK::VersionNumber /* VersionNumber */) override { return false; }

    void doPreWrite(TransferType, VersionNumber) override {
      throw ChimeraTK::logic_error("NumericAddressBackend DoubleBufferPlugin: Writing is not allowed atm.");
    }

    void doPostWrite(TransferType, VersionNumber) override {
      // do not throw here again
    }
    // below functions are needed for TransferGroup to work
    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override;

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override { return {}; }

    void replaceTransferElement(boost::shared_ptr<ChimeraTK::TransferElement> /* newElement */) override {}
    [[nodiscard]] bool mayReplaceOther(const boost::shared_ptr<TransferElement const>& other) const override;

   private:
    using ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D;
    NumericAddressedRegisterInfo::DoubleBufferInfo _doubleBufferInfo;
    boost::shared_ptr<DeviceBackend> _backend;
    std::shared_ptr<detail::CountedRecursiveMutex> _mutex;
    std::unique_lock<detail::CountedRecursiveMutex> _transferLock;
    boost::shared_ptr<NDRegisterAccessor<UserType>> _buffer0;
    boost::shared_ptr<NDRegisterAccessor<UserType>> _buffer1;
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<uint32_t>> _enableDoubleBufferReg;
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<uint32_t>> _currentBufferNumberReg;
    uint32_t _currentBuffer{0};
    const RegisterPath& _registerPathName;
    size_t _numberOfWords;
    size_t _wordOffsetInRegister;
    AccessModeFlags _flags;
  };

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(DoubleBufferAccessorDecorator);
} // namespace ChimeraTK
