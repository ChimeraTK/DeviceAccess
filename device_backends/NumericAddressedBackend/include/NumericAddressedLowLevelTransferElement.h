// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NumericAddressedBackend.h"
#include "TransferElement.h"

namespace ChimeraTK {

  template<typename UserType, typename DataConverterType, bool isRaw>
  class NumericAddressedBackendRegisterAccessor;

  class NumericAddressedBackendASCIIAccessor;

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
        boost::shared_ptr<NumericAddressedBackend> dev, size_t bar, size_t startAddress, size_t numberOfBytes)
    : TransferElement("", {AccessMode::raw}), _dev(dev), _bar(bar), isShared(false),
      _unalignedAccess(_dev->_unalignedAccess, std::defer_lock) {
      if(!dev->barIndexValid(bar)) {
        std::stringstream errorMessage;
        errorMessage << "Invalid bar number: " << bar << std::endl;
        throw ChimeraTK::logic_error(errorMessage.str());
      }
      setAddress(startAddress, numberOfBytes);
    }

    virtual ~NumericAddressedLowLevelTransferElement() {}

    void doReadTransferSynchronously() override {
      _dev->read(_bar, _startAddress, reinterpret_cast<int32_t*>(rawDataBuffer.data()), _numberOfBytes);
    }

    bool doWriteTransfer(ChimeraTK::VersionNumber) override {
      _dev->write(_bar, _startAddress, reinterpret_cast<int32_t*>(rawDataBuffer.data()), _numberOfBytes);
      return false;
    }

    void doPostRead(TransferType, bool hasNewData) override {
      if(hasNewData) {
        // it is acceptable to create a new version number only in doPostRead because the LowLevelTransferElement never
        // has wait_for_new_data.
        _versionNumber = {};
      }
    }

    void doPreWrite(TransferType, VersionNumber) override {
      if(_isUnaligned) {
        _unalignedAccess.lock();
        _dev->read(_bar, _startAddress, reinterpret_cast<int32_t*>(rawDataBuffer.data()), _numberOfBytes);
      }
    }

    void doPostWrite(TransferType, VersionNumber) override {
      if(_unalignedAccess.owns_lock()) {
        _unalignedAccess.unlock();
      }
    }

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

    /** Return pointer to the begin of the raw buffer matching the given address.
     * No end() is provided, since the NumericAddressedBackendRegisterAccessor
     * using this functionality uses the cooked buffer for this check. Only
     * addresses within the range specified in the constructor or changeAddress()
     * may be passed. The address must also have an integer multiple of the word
     * size as an offset w.r.t. the start address specified in the constructor.
     * Otherwise an undefined behaviour will occur! */
    uint8_t* begin(size_t addressInBar) { return rawDataBuffer.data() + (addressInBar - _startAddress); }

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

   protected:
    /** Set the start address (inside the bar given in the constructor) and number
     * of words of this accessor. */
    void setAddress(size_t startAddress, size_t numberOfBytes) {
      // change address
      _startAddress = startAddress;
      _numberOfBytes = numberOfBytes;

      // make sure access is properly aligned
      _isUnaligned = false;
      auto alignment = _dev->minimumTransferAlignment(_bar);
      auto start_padding = _startAddress % alignment;
      assert(_startAddress >= start_padding);
      if(start_padding > 0) _isUnaligned = true;
      _startAddress -= start_padding;
      _numberOfBytes += start_padding;
      auto end_padding = alignment - _numberOfBytes % alignment;
      if(end_padding != alignment) {
        _isUnaligned = true;
        _numberOfBytes += end_padding;
      }

      // Allocated the buffer
      rawDataBuffer.resize(_numberOfBytes);

      // update the name
      _name = "NALLTE:" + std::to_string(_startAddress) + "+" + std::to_string(_numberOfBytes);
    }

    /** the backend to use for the actual hardware access */
    boost::shared_ptr<NumericAddressedBackend> _dev;

    /** start address w.r.t. the PCIe bar */
    uint64_t _bar;

    /** start address w.r.t. the PCIe bar */
    uint64_t _startAddress;

    /** number of bytes to access */
    size_t _numberOfBytes;

    /** flag if changeAddress() has been called, which is this low-level transfer
     * element is shared between multiple accessors */
    bool isShared;

    /** flag whether access is unaligned */
    bool _isUnaligned{false};

    /** Lock to protect unaligned access (with mutex from backend) */
    std::unique_lock<std::mutex> _unalignedAccess;

    /** raw buffer */
    std::vector<uint8_t> rawDataBuffer;

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
      return {boost::enable_shared_from_this<TransferElement>::shared_from_this()};
    }

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override { return {}; }

    void replaceTransferElement(boost::shared_ptr<TransferElement> /*newElement*/) override {} // LCOV_EXCL_LINE

    template<typename UserType, typename DataConverterType, bool isRaw>
    friend class NumericAddressedBackendRegisterAccessor;

    template<typename UserType, typename DataConverterType, bool isRaw>
    friend struct detail::NumericAddressedPrePostActionsImplementor;

    friend class NumericAddressedBackendASCIIAccessor;
  };

} // namespace ChimeraTK
