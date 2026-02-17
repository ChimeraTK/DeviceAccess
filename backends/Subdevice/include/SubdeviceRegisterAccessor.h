// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NDRegisterAccessor.h"
#include "SubdeviceBackend.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<typename RegisterRawType>
  class SubdeviceRegisterAccessor : public NDRegisterAccessor<RegisterRawType> {
   public:
    SubdeviceRegisterAccessor(boost::shared_ptr<SubdeviceBackend> backend, const std::string& registerPathName,
        boost::shared_ptr<NDRegisterAccessor<uint32_t>> accChipSelect,
        boost::shared_ptr<NDRegisterAccessor<uint32_t>> accAddress,
        boost::shared_ptr<NDRegisterAccessor<uint32_t>> accWriteDataArea,
        boost::shared_ptr<NDRegisterAccessor<uint32_t>> accStatus,
        boost::shared_ptr<NDRegisterAccessor<ChimeraTK::Void>> accReadRequest,
        boost::shared_ptr<NDRegisterAccessor<uint32_t>> accReadData, size_t byteOffset, size_t numberOfWords);

    void doReadTransferSynchronously() override;

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber) override;

    void doPreRead(TransferType type) override;

    void doPostRead(TransferType type, bool hasNewData) override;

    void doPreWrite(TransferType type, VersionNumber) override;

    void doPostWrite(TransferType type, VersionNumber) override;

    bool mayReplaceOther(const boost::shared_ptr<TransferElement const>&) const override;

    bool isReadOnly() const override;

    bool isReadable() const override;

    bool isWriteable() const override;

   protected:
    /// Pointer to the backend
    boost::shared_ptr<SubdeviceBackend> _backend;

    /// Pointers to the accessors
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _accChipSelect;         // chip select register, if present
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _accAddress;            // address register, if present
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _accWriteDataArea;      // write data or write area register
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _accStatus;             // status register, if present
    boost::shared_ptr<NDRegisterAccessor<ChimeraTK::Void>> _accReadRequest; // read request register, if present
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _accReadData;           // read data (area not supported)

    /// start address and length
    size_t _startAddress, _numberOfWords;

    /// internal buffer
    std::vector<RegisterRawType> _buffer; // Fixme: unclear if this should be byte

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override;

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override;

    void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override;
  };

} // namespace ChimeraTK
