/*
 * NumericAddressedBackendRawAccessor.h
 *
 *  Created on: Aug 8, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_NUMERIC_ADDRESSED_BACKEND_RAW_ACCESSOR_H
#define MTCA4U_NUMERIC_ADDRESSED_BACKEND_RAW_ACCESSOR_H

#include "TransferElement.h"
#include "NumericAddressedBackend.h"
#include "FixedPointConverter.h"

namespace mtca4u {

  class DeviceBackend;

  template<typename UserType>
  class NumericAddressedBackendRegisterAccessor;

  /*********************************************************************************************************************/
  /** Implementation of the NDRegisterAccessor for NumericAddressedBackends, responsible for the underlying raw data
   *  access. This accessor is never directly returned to the user and thus is based only on the TransferElement base
   *  class (unstead of the NDRegisterAccessor). It is only internally used by other register accessors of the
   *  NumericAddressBackends. The reason for introducing this class is that it allows the TransferGroup to replace
   *  the raw accessor used by other accessors to merge data transfers of neighbouring registers.
   */
  class NumericAddressedBackendRawAccessor : public TransferElement {
    public:

      NumericAddressedBackendRawAccessor(boost::shared_ptr<NumericAddressedBackend> dev,
          size_t bar, size_t startAddress, size_t numberOfWords)
      : _dev(dev), _bar(bar), _startAddress(startAddress), _numberOfWords(numberOfWords)
      {
        // allocated the buffer
        rawDataBuffer.resize(_numberOfWords);

        // compute number of bytes
        _numberOfBytes = _numberOfWords*sizeof(int32_t);
      }

      virtual ~NumericAddressedBackendRawAccessor() {};

      virtual void read() {
        _dev->read(_bar, _startAddress, rawDataBuffer.data(), _numberOfBytes);
      }

      virtual void write() {
        _dev->write(_bar, _startAddress, rawDataBuffer.data(), _numberOfBytes);
      }

      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {
        auto rhsCasted = boost::dynamic_pointer_cast< const NumericAddressedBackendRawAccessor >(other);
        if(!rhsCasted) return false;
        if(_dev != rhsCasted->_dev) return false;
        if(_bar != rhsCasted->_bar) return false;
        if(_startAddress != rhsCasted->_startAddress) return false;
        if(_numberOfWords != rhsCasted->_numberOfWords) return false;
        return true;
      }

      virtual bool isReadOnly() const {
        return false;
      }

    protected:

      /** the backend to use for the actual hardware access */
      boost::shared_ptr<NumericAddressedBackend> _dev;

      /** start address w.r.t. the PCIe bar */
      size_t _bar;

      /** start address w.r.t. the PCIe bar */
      size_t _startAddress;

      /** number of 4-byte words to access */
      size_t _numberOfWords;

      /** number of bytes to access */
      size_t _numberOfBytes;

      /** raw buffer */
      std::vector<int32_t> rawDataBuffer;

      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() {
        return { boost::enable_shared_from_this<TransferElement>::shared_from_this() };
      }

      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> /*newElement*/) {} // LCOV_EXCL_LINE

      template<typename UserType>
      friend class NumericAddressedBackendRegisterAccessor;

  };

}    // namespace mtca4u

#endif /* MTCA4U_NUMERIC_ADDRESSED_BACKEND_RAW_ACCESSOR_H */
