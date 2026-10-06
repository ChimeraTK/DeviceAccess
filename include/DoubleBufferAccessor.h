// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NDRegisterAccessor.h"
#include "NDRegisterAccessorDecorator.h"
#include "NumericAddressedBackend.h"
#include "RegisterInfo.h"
#include "ScalarRegisterAccessor.h"
#include "TransferElement.h"

#include <string>

namespace ChimeraTK {

  template<typename UserType>
  class DoubleBufferAccessor : public NDRegisterAccessor<UserType> {
   public:
    DoubleBufferAccessor(NumericAddressedRegisterInfo::DoubleBufferInfo doubleBufferConfig,
        const boost::shared_ptr<DeviceBackend>& backend, std::shared_ptr<detail::CountedRecursiveMutex> mutex,
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags,
        const SelectedBy* selectedBy = nullptr);

    /** Enable or disable the skip-on-unselected behaviour: if enabled and the register's selection gate is closed,
     *  doReadTransferSynchronously() skips the physical buffer read entirely (used on the interrupt/async path so an
     *  unselected double-buffered register is not read from hardware). The selector register is still read so a change
     *  of selection is detected on a later interrupt. The wrapped data is reported faulty/not-new in that case, so the
     *  delivery is suppressed without touching stale hardware buffers. */
    void setSkipOnUnselected(bool skip = true) override { _skipWhenUnselected = skip; }

    void doPreRead(TransferType type) override;

    void doReadTransferSynchronously() override;

    void doPostRead(TransferType type, bool hasNewData) override;

    /**
     * Whether the selector register currently selects the requested gate. Returns the most recent decision made
     * during the last transfer cycle (no selector re-read). true when no selector is attached.
     */
    [[nodiscard]] bool isSelected() const { return !_selectorAccessor || _lastGateOpen; }

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
    boost::shared_ptr<DeviceBackend> _backend;
    std::shared_ptr<detail::CountedRecursiveMutex> _mutex;
    std::unique_lock<detail::CountedRecursiveMutex> _transferLock;
    boost::shared_ptr<NDRegisterAccessor<UserType>> _buffer0;
    boost::shared_ptr<NDRegisterAccessor<UserType>> _buffer1;
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<uint32_t>> _enableDoubleBufferReg;
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<uint32_t>> _currentBufferNumberReg;
    uint32_t _currentBuffer{0};

    /// The scalar accessor reading the 'selectedBy' selector register (owned by this accessor, when a gate is set).
    boost::shared_ptr<ScalarRegisterAccessor<int64_t>> _selectorAccessor;
    int64_t _expectedValue{0};

    /// Whether doReadTransferSynchronously() shall skip the physical data read when the gate is closed. Enabled by
    /// the interrupt (async) path; disabled in the polled path so the physical read always happens.
    bool _skipWhenUnselected{false};

    /// Set when doReadTransferSynchronously() skipped the data read because the gate is closed. Consumed by
    /// doPostRead() so it does not forward postRead to the (unread) buffer accessor.
    bool _transferSkipped{false};

    /// Most recent gate decision (persistent across transfer cycles), queried by isSelected().
    bool _lastGateOpen{true};
  };

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(DoubleBufferAccessor);
} // namespace ChimeraTK
