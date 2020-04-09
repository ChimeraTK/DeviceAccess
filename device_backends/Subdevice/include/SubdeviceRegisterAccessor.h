/*
 * SubdeviceRegisterAccessor.h
 *
 *  Created on: Dec 10, 2018
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_SUBDEVICE_REGISTER_ACCESSOR_H
#define CHIMERA_TK_SUBDEVICE_REGISTER_ACCESSOR_H

#include <algorithm>

#include "Device.h"
#include "SubdeviceBackend.h"
#include "SyncNDRegisterAccessor.h"
#include "TransferElement.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  class SubdeviceRegisterAccessor : public SyncNDRegisterAccessor<int32_t> {
   public:
    SubdeviceRegisterAccessor(boost::shared_ptr<SubdeviceBackend> backend, const std::string& registerPathName,
        boost::shared_ptr<NDRegisterAccessor<int32_t>> accAddress,
        boost::shared_ptr<NDRegisterAccessor<int32_t>> accData,
        boost::shared_ptr<NDRegisterAccessor<int32_t>> accStatus, size_t byteOffset, size_t numberOfWords);

    ~SubdeviceRegisterAccessor() override;

    void doReadTransfer(TransferType type) override;

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber = {}) override;

    bool doReadTransferNonBlocking(TransferType type) override;

    bool doReadTransferLatest(TransferType type) override;

    void doPreRead() override;

    void doPostRead() override;

    void doPreWrite() override;

    void doPostWrite() override;

    bool mayReplaceOther(const boost::shared_ptr<TransferElement const>&) const override;

    bool isReadOnly() const override;

    bool isReadable() const override;

    bool isWriteable() const override;

    AccessModeFlags getAccessModeFlags() const override;

    VersionNumber getVersionNumber() const override { return _accData->getVersionNumber(); }

   protected:
    /// Pointer to the backend
    boost::shared_ptr<SubdeviceBackend> _backend;

    /// Pointers to the three accessors
    boost::shared_ptr<NDRegisterAccessor<int32_t>> _accAddress;
    boost::shared_ptr<NDRegisterAccessor<int32_t>> _accData;
    boost::shared_ptr<NDRegisterAccessor<int32_t>> _accStatus;

    /// start address and length
    size_t _byteOffset, _numberOfWords;

    /// internal buffer
    std::vector<int32_t> _buffer;

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override;

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override;

    void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override;
  };

} // namespace ChimeraTK

#endif /* CHIMERA_TK_SUBDEVICE_REGISTER_ACCESSOR_H */
