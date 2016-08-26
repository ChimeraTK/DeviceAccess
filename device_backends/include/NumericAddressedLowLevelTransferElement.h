/*
 * NumericAddressedLowLevelTransferElement.h
 *
 *  Created on: Aug 8, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_NUMERIC_ADDRESSED_LOW_LEVEL_TRANSFER_ELEMENT_H
#define MTCA4U_NUMERIC_ADDRESSED_LOW_LEVEL_TRANSFER_ELEMENT_H

#include "TransferElement.h"
#include "NumericAddressedBackend.h"
#include "FixedPointConverter.h"

namespace mtca4u {

  template<typename UserType>
  class NumericAddressedBackendRegisterAccessor;

  /*********************************************************************************************************************/
  /** Implementation of the NDRegisterAccessor for NumericAddressedBackends, responsible for the underlying raw data
   *  access. This accessor is never directly returned to the user and thus is based only on the TransferElement base
   *  class (unstead of the NDRegisterAccessor). It is only internally used by other register accessors of the
   *  NumericAddressBackends. The reason for introducing this class is that it allows the TransferGroup to replace
   *  the raw accessor used by other accessors to merge data transfers of neighbouring registers.
   */
  class NumericAddressedLowLevelTransferElement : public TransferElement {
    public:

      NumericAddressedLowLevelTransferElement(boost::shared_ptr<NumericAddressedBackend> dev,
          size_t bar, size_t startAddress, size_t numberOfWords)
      : _dev(dev), _bar(bar)
      {
        changeAddress(startAddress, numberOfWords);
      }

      virtual ~NumericAddressedLowLevelTransferElement() {};

      virtual void read() {
        _dev->read(_bar, _startAddress, rawDataBuffer.data(), _numberOfBytes);
      }

      virtual void write() {
        _dev->write(_bar, _startAddress, rawDataBuffer.data(), _numberOfBytes);
      }

      virtual bool readNonBlocking(){
	throw DeviceException("Non-blocking read is not available for NumericAddressedBackends",
			      DeviceException::NOT_AVAILABLE);
      }

      /** Check if the two TransferElements are identical, i.e. accessing the same hardware register. In the special
       *  case of the NumericAddressedBackendRawAccessor, this function returns also true if the address areas
       *  are adjacent and/or overlapping. NumericAddressedBackendRegisterAccessor::replaceTransferElement() takes
       *  care of replacing the NumericAddressedBackendRawAccessors with a single NumericAddressedBackendRawAccessor
       *  covering the address space of both accessors. */
      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {
        // accessor type, device and bar must be the same
        auto rhsCasted = boost::dynamic_pointer_cast< const NumericAddressedLowLevelTransferElement >(other);
        if(!rhsCasted) return false;
        if(_dev != rhsCasted->_dev) return false;
        if(_bar != rhsCasted->_bar) return false;

        // only allow adjacent and overlapping address areas to be merged
        if(_startAddress + _numberOfBytes < rhsCasted->_startAddress) return false;
        if(_startAddress > rhsCasted->_startAddress + rhsCasted->_numberOfBytes) return false;
        return true;
      }

      virtual const std::type_info& getValueType() const{
	// This implementation is for int32_t only (as all numerically addressed backends under the
	// hood.
	return typeid(int32_t);
      }

      virtual bool isReadOnly() const {
        return false;
      }

      virtual bool isReadable() const {
        return true;
      }

      virtual bool isWriteable() const {
        return true;
      }

      /** Return accessor to the begin of the raw buffer matching the given address. No end() is provided, since the
       *  NumericAddressedBackendRegisterAccessor using this functionality uses the cooked buffer for this check.
       *  Only addresses within the range specified in the constructor may be passed. The address must also have an
       *  integer multiple of the word size as an offset w.r.t. the start address specified in the constructor.
       *  Otherwise an undefined behaviour will occur! */
      std::vector<int32_t>::iterator begin(size_t addressInBar) {
        return rawDataBuffer.begin() + (addressInBar-_startAddress)/sizeof(int32_t);
      }

      /** Change the start address (inside the bar given in the constructor) and number of words of this accessor. */
      void changeAddress(size_t startAddress, size_t numberOfWords) {

        // change address
        _startAddress = startAddress;
        _numberOfWords = numberOfWords;

        // allocated the buffer
        rawDataBuffer.resize(_numberOfWords);

        // compute number of bytes
        _numberOfBytes = _numberOfWords*sizeof(int32_t);
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

#endif /* MTCA4U_NUMERIC_ADDRESSED_LOW_LEVEL_TRANSFER_ELEMENT_H */
