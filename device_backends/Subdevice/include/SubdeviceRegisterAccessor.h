// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NDRegisterAccessor.h"
#include "SubdeviceBackend.h"

#include <algorithm>

namespace ChimeraTK {

  /*********************************************************************************************************************/

  class SubdeviceRegisterAccessor : public NDRegisterAccessor<int32_t> {
   public:
    SubdeviceRegisterAccessor(boost::shared_ptr<SubdeviceBackend> backend, const std::string& registerPathName,
        boost::shared_ptr<NDRegisterAccessor<int32_t>> accAddress,
        boost::shared_ptr<NDRegisterAccessor<int32_t>> accDataArea,
        boost::shared_ptr<NDRegisterAccessor<int32_t>> accStatus, size_t byteOffset, size_t numberOfWords);

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

    /// Pointers to the three accessors
    boost::shared_ptr<NDRegisterAccessor<int32_t>> _accAddress;  // address register, if present
    boost::shared_ptr<NDRegisterAccessor<int32_t>> _accDataArea; // data or area register
    boost::shared_ptr<NDRegisterAccessor<int32_t>> _accStatus;   // status register, if present

    /// start address and length
    size_t _startAddress, _numberOfWords;

    /// internal buffer
    std::vector<int32_t> _buffer;

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override;

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override;

    void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override;
  };

} // namespace ChimeraTK
