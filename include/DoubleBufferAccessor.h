// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "ClosedGateQuery.h"
#include "NDRegisterAccessor.h"
#include "NDRegisterAccessorDecorator.h"
#include "NumericAddressedBackend.h"
#include "RegisterInfo.h"
#include "TransferElement.h"

#include <string>

namespace ChimeraTK {

  template<typename UserType>
  class DoubleBufferAccessor : public NDRegisterAccessor<UserType>, public ClosedGateQuery {
   public:
    DoubleBufferAccessor(NumericAddressedRegisterInfo::DoubleBufferInfo doubleBufferConfig,
        const boost::shared_ptr<DeviceBackend>& backend, std::shared_ptr<detail::CountedRecursiveMutex> mutex,
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags,
        std::optional<NumericAddressedRegisterInfo::GatedBy> gatedBy = std::nullopt);

    void doPreRead(TransferType type) override;

    void doReadTransferSynchronously() override;

    void doPostRead(TransferType type, bool hasNewData) override;

    [[nodiscard]] bool isGateClosed() const override;

    [[nodiscard]] bool isWriteable() const override { return false; }
    [[nodiscard]] bool isReadOnly() const override { return true; }
    [[nodiscard]] bool isReadable() const override { return true; };

    bool doWriteTransfer(ChimeraTK::VersionNumber /* VersionNumber */) override { return false; }

    void doPreWrite(TransferType, VersionNumber) override {
      throw ChimeraTK::logic_error("DoubleBufferAccessor: Writing is not allowed atm.");
    }

    void doPostWrite(TransferType, VersionNumber) override {
      // do not throw here again
    }

    // below functions are needed for TransferGroup to work
    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override;

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override { return {}; }

    void replaceTransferElement(boost::shared_ptr<ChimeraTK::TransferElement> /* newElement */) override {}
    [[nodiscard]] bool mayReplaceOther(const boost::shared_ptr<TransferElement const>& other) const override;

   protected:
    using ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D;
    NumericAddressedRegisterInfo::DoubleBufferInfo _doubleBufferInfo;
    boost::shared_ptr<NumericAddressedBackend> _backend;
    std::shared_ptr<detail::CountedRecursiveMutex> _mutex;
    std::unique_lock<detail::CountedRecursiveMutex> _transferLock;
    boost::shared_ptr<NDRegisterAccessor<UserType>> _buffer0;
    boost::shared_ptr<NDRegisterAccessor<UserType>> _buffer1;
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<uint32_t>> _enableDoubleBufferReg;
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<uint32_t>> _currentBufferNumberReg;
    uint32_t _currentBuffer{0};

    /**
     * The gating register accessor of this register's gate (a register-level gate of a 2D register, or the gate of a
     * scalar/1D register or a double-buffered channel slice), or null when the register carries no gate. It is read in
     * the transfer phase, next to the buffer read.
     */
    boost::shared_ptr<NDRegisterAccessor<uint64_t>> _gateAccessor;

    /** The expected value of the gating register which opens the gate. */
    uint64_t _gateValue{0};

    /**
     * The gate result of the inner buffer most recently read: true while the gate is closed. Taken from the current
     * buffer's transfer, so isGateClosed() answers for the currently active inner buffer.
     */
    bool _gateClosed{false};
  };

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(DoubleBufferAccessor);
} // namespace ChimeraTK
