/*
 * NumericAddressedLowLevelTransferElement.h
 *
 *  Created on: Aug 8, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_NUMERIC_ADDRESSED_LOW_LEVEL_TRANSFER_ELEMENT_H
#define CHIMERA_TK_NUMERIC_ADDRESSED_LOW_LEVEL_TRANSFER_ELEMENT_H

#include "NumericAddressedBackend.h"
#include "TransferElement.h"

namespace ChimeraTK {

  template<typename UserType, typename DataConverterType, bool isRaw>
  class NumericAddressedBackendRegisterAccessor;

  namespace detail {
    template<typename UserType, typename DataConverterType, bool isRaw>
    struct NumericAddressedPrePostActionsImplementor;
  }

  /*********************************************************************************************************************/
  /** Implementation of the NDRegisterAccessor for NumericAddressedBackends,
   * responsible for the underlying raw data access. This accessor is never
   * directly returned to the user and thus is based only on the TransferElement
   * base class (unstead of the NDRegisterAccessor). It is only internally used by
   * other register accessors of the NumericAddressBackends. The reason for
   * introducing this class is that it allows the TransferGroup to replace the raw
   * accessor used by other accessors to merge data transfers of neighbouring
   * registers.
   */
  class NumericAddressedLowLevelTransferElement : public TransferElement {
   public:
    NumericAddressedLowLevelTransferElement(
        boost::shared_ptr<NumericAddressedBackend> dev, size_t bar, size_t startAddress, size_t numberOfWords)
    : _dev(dev), _bar(bar), isShared(false) {
      setAddress(startAddress, numberOfWords);
    }

    virtual ~NumericAddressedLowLevelTransferElement() {}

    void doReadTransfer() override { _dev->read(_bar, _startAddress, rawDataBuffer.data(), _numberOfBytes); }

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber = {}) override {
      _dev->write(_bar, _startAddress, rawDataBuffer.data(), _numberOfBytes);
      currentVersion = versionNumber;
      return false;
    }

    bool doReadTransferNonBlocking() override {
      doReadTransfer();
      return true;
    }

    bool doReadTransferLatest() override {
      doReadTransfer();
      return true;
    }

    void doPostRead() override { currentVersion = {}; }

    TransferFuture doReadTransferAsync() override { // LCOV_EXCL_LINE
      // This function is not needed and will never be called. If readAsync() is
      // called on the high-level accessor, the transfer will be "backgrounded"
      // already on that level.
      throw ChimeraTK::logic_error("NumericAddressedLowLevelTransferElement::readAsync() is not "
                                   "implemented"); // LCOV_EXCL_LINE
    }                                              // LCOV_EXCL_LINE

    /** Check if the address areas are adjacent and/or overlapping.
     *  NumericAddressedBackendRegisterAccessor::replaceTransferElement() takes
     * care of replacing the NumericAddressedBackendRawAccessors with a single
     * NumericAddressedBackendRawAccessor covering the address space of both
     * accessors. */
    bool isMergeable(const boost::shared_ptr<TransferElement const>& other) const {
      if(!_dev->canMergeRequests()) return false;

      // accessor type, device and bar must be the same
      auto rhsCasted = boost::dynamic_pointer_cast<const NumericAddressedLowLevelTransferElement>(other);
      if(!rhsCasted) return false;
      if(_dev != rhsCasted->_dev) return false;
      if(_bar != rhsCasted->_bar) return false;

      // only allow adjacent and overlapping address areas to be merged
      if(_startAddress + _numberOfBytes < rhsCasted->_startAddress) return false;
      if(_startAddress > rhsCasted->_startAddress + rhsCasted->_numberOfBytes) return false;
      return true;
    }

    bool mayReplaceOther(const boost::shared_ptr<TransferElement const>&) const override {
      return false; // never used, since isMergeable() is used instead
    }

    const std::type_info& getValueType() const override {
      // This implementation is for int32_t only (as all numerically addressed
      // backends under the hood.
      return typeid(int32_t);
    }

    bool isReadOnly() const override { return false; }

    bool isReadable() const override { return true; }

    bool isWriteable() const override { return true; }

    /** Return accessor to the begin of the raw buffer matching the given address.
     * No end() is provided, since the NumericAddressedBackendRegisterAccessor
     * using this functionality uses the cooked buffer for this check. Only
     * addresses within the range specified in the constructor or changeAddress()
     * may be passed. The address must also have an integer multiple of the word
     * size as an offset w.r.t. the start address specified in the constructor.
     * Otherwise an undefined behaviour will occur! */
    std::vector<int32_t>::iterator begin(size_t addressInBar) {
      return rawDataBuffer.begin() + (addressInBar - _startAddress) / sizeof(int32_t);
    }

    /** Change the start address (inside the bar given in the constructor) and
     * number of words of this accessor,  and set the shared flag. */
    void changeAddress(size_t startAddress, size_t numberOfWords) {
      setAddress(startAddress, numberOfWords);
      isShared = true;
    }

    boost::shared_ptr<TransferElement> makeCopyRegisterDecorator() override { // LCOV_EXCL_LINE
      throw ChimeraTK::logic_error("NumericAddressedLowLevelTransferElement::makeCopyRegisterDecorator() "
                                   "is not implemented"); // LCOV_EXCL_LINE
    }                                                     // LCOV_EXCL_LINE

    AccessModeFlags getAccessModeFlags() const override { return {AccessMode::raw}; }

    VersionNumber getVersionNumber() const override { return currentVersion; }

   protected:
    /** Set the start address (inside the bar given in the constructor) and number
     * of words of this accessor. */
    void setAddress(size_t startAddress, size_t numberOfWords) {
      // change address
      _startAddress = startAddress;
      _numberOfWords = numberOfWords;

      // allocated the buffer
      rawDataBuffer.resize(_numberOfWords);

      // compute number of bytes
      _numberOfBytes = _numberOfWords * sizeof(int32_t);

      // update the name
      _name = "NALLTE:" + std::to_string(_startAddress) + "+" + std::to_string(_numberOfBytes);
    }

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

    /** flag if changeAddress() has been called, which is this low-level transfer
     * element is shared between multiple accessors */
    bool isShared;

    /** raw buffer */
    std::vector<int32_t> rawDataBuffer;

    /** version number of the last transfer */
    VersionNumber currentVersion;

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
      return {boost::enable_shared_from_this<TransferElement>::shared_from_this()};
    }

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override { return {}; }

    void replaceTransferElement(boost::shared_ptr<TransferElement> /*newElement*/) override {} // LCOV_EXCL_LINE

    template<typename UserType, typename DataConverterType, bool isRaw>
    friend class NumericAddressedBackendRegisterAccessor;
    template<typename UserType, typename DataConverterType, bool isRaw>
    friend struct detail::NumericAddressedPrePostActionsImplementor;
  };

} // namespace ChimeraTK

#endif /* CHIMERA_TK_NUMERIC_ADDRESSED_LOW_LEVEL_TRANSFER_ELEMENT_H */
