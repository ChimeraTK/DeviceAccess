// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NDRegisterAccessor.h"
#include "SubdeviceBackend.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  /// The RegisterRawType is determined by the number of bytes in the SubDevice mapfile
  /// The ReadWriteDataType is determined by the number of bits in the target ReadData and WriteData registers.
  template<typename RegisterRawType, typename ReadWriteDataType>
  class SubdeviceRegisterWindowAccessor : public NDRegisterAccessor<RegisterRawType> {
   public:
    SubdeviceRegisterWindowAccessor(boost::shared_ptr<SubdeviceBackend> backend, const std::string& registerPathName,
        size_t numberOfWords, size_t wordOffsetInRegister);

    void doReadTransferSynchronously() override;

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber) override;

    void doPreRead(TransferType type) override;

    void doPostRead(TransferType type, bool hasNewData) override;

    void doPreWrite(TransferType type, VersionNumber) override;

    void doPostWrite(TransferType type, VersionNumber) override;

    [[nodiscard]] bool mayReplaceOther(const boost::shared_ptr<TransferElement const>&) const override;

    [[nodiscard]] bool isReadOnly() const override;

    [[nodiscard]] bool isReadable() const override;

    [[nodiscard]] bool isWriteable() const override;

   protected:
    /// Pointer to the backend
    boost::shared_ptr<SubdeviceBackend> _backend;

    /// Pointers to the accessors
    boost::shared_ptr<NDRegisterAccessor<uint64_t>> _accChipSelect;         // chip select register, if present
    boost::shared_ptr<NDRegisterAccessor<uint64_t>> _accAddress;            // address register, if present
    boost::shared_ptr<NDRegisterAccessor<ReadWriteDataType>> _accWriteData; // write data or write area register
    boost::shared_ptr<NDRegisterAccessor<ChimeraTK::Boolean>> _accBusy;     // status register, if present
    boost::shared_ptr<NDRegisterAccessor<ChimeraTK::Void>> _accReadRequest; // read request register, if present
    boost::shared_ptr<NDRegisterAccessor<ReadWriteDataType>> _accReadData;  // read data (area not supported)

    // cached values needed in each transfer
    size_t _startAddress, _endAddress, _transferSize; // in bytes
    size_t _starTransferAddress, _endTransferAddress; // in increments of _transfersize, not in bytes
    size_t _numberOfWords;

    /// internal buffer
    std::vector<RegisterRawType> _buffer; // Fixme: unclear if this should be byte

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override;

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override;

    void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override;

    enum class TransferDirection { read, write };
    // Helper for the transfer to minimise code duplication
    void transferImpl(TransferDirection direction);
    std::vector<std::byte> _zeros; // _transferSize bytes with 0 to copy from for padding
  };

} // namespace ChimeraTK
