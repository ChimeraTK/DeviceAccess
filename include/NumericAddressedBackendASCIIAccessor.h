// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "ForwardDeclarations.h"
#include "NDRegisterAccessor.h"
#include "NumericAddressedLowLevelTransferElement.h"
#include "ReadCoordinator.h"

#include <ChimeraTK/cppext/finally.hpp>

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * Implementation of the NDRegisterAccessor for NumericAddressedBackends for ASCII data
   */
  class NumericAddressedBackendASCIIAccessor : public NDRegisterAccessor<std::string> {
   public:
    NumericAddressedBackendASCIIAccessor(const boost::shared_ptr<DeviceBackend>& dev,
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister,
        const AccessModeFlags& flags)
    : NDRegisterAccessor<std::string>(registerPathName, flags),
      _dev(boost::dynamic_pointer_cast<NumericAddressedBackend>(dev)) {
      // check for unknown flags
      flags.checkForUnknownFlags({});

      // check device backend
      _dev = boost::dynamic_pointer_cast<NumericAddressedBackend>(dev);
      if(!_dev) {
        throw ChimeraTK::logic_error("NumericAddressedBackendASCIIAccessor is used with a backend which is not "
                                     "a NumericAddressedBackend.");
      }

      // obtain register information
      _registerInfo = _dev->getRegisterInfo(registerPathName);
      assert(!_registerInfo.channels.empty());

      if(_registerInfo.elementPitchBits % 8 != 0) {
        throw ChimeraTK::logic_error("NumericAddressedBackendASCIIAccessor: Elements must be byte aligned.");
      }

      if(_registerInfo.channels.size() > 1) {
        throw ChimeraTK::logic_error("NumericAddressedBackendASCIIAccessor is used with a 2D register.");
      }

      if(_registerInfo.channels.front().bitOffset > 0) {
        throw ChimeraTK::logic_error("NumericAddressedBackendASCIIAccessor: Registers must be byte aligned.");
      }

      if(_registerInfo.channels.front().dataType != NumericAddressedRegisterInfo::Type::ASCII) {
        throw ChimeraTK::logic_error("NumericAddressedBackendASCIIAccessor: Cannot be used with non-ASCII registers.");
      }

      // check number of words
      if(numberOfWords == 0) {
        numberOfWords = _registerInfo.getNumberOfElements();
      }
      if(numberOfWords + wordOffsetInRegister > _registerInfo.getNumberOfElements()) {
        throw ChimeraTK::logic_error("Requested number of words (" + std::to_string(numberOfWords) + " + " +
            std::to_string(wordOffsetInRegister) + ") exceeds the size (" +
            std::to_string(_registerInfo.getNumberOfElements()) + ") of the register '" + registerPathName + "'!");
      }
      if(wordOffsetInRegister >= _registerInfo.getNumberOfElements()) {
        throw ChimeraTK::logic_error("Requested offset (" + std::to_string(wordOffsetInRegister) +
            ") exceeds the size (" + std::to_string(_registerInfo.getNumberOfElements()) + ") of the register'" +
            registerPathName + "'!");
      }

      // change registerInfo (local copy!) to account for given offset and length override
      _registerInfo.address += wordOffsetInRegister * _registerInfo.elementPitchBits / 8;
      _registerInfo.nElements = numberOfWords;

      // Record the gate of this read consumer, if the register carries one. The gate accessor is not built here: the
      // low-level element owns it and builds it on demand, keyed by the gating register path. The gating register must
      // be scalar; the parser already rejects a gating register that is itself gated, so no gate chain and hence no
      // recursion occurs. The scalar precondition is checked here, when the gated accessor is created.
      if(_registerInfo.channels.front().gatedBy.has_value()) {
        const auto& gatedBy = *_registerInfo.channels.front().gatedBy;
        auto gateInfo = _dev->getRegisterInfo(gatedBy.regPath);
        if(gateInfo.getNumberOfChannels() != 1 || gateInfo.nElements > 1) {
          throw ChimeraTK::logic_error("gatedBy: the gating register '" + gatedBy.regPath + "' of register '" +
              registerPathName + "' must be a scalar register.");
        }
        _gateValue = gatedBy.val;
      }

      // create low-level transfer element handling the actual data transfer to the hardware with raw data
      assert(_registerInfo.elementPitchBits % 8 == 0);
      _rawAccessor = boost::make_shared<NumericAddressedLowLevelTransferElement>(
          _dev, _registerInfo.bar, _registerInfo.address, _registerInfo.nElements * _registerInfo.elementPitchBits / 8);

      // allocated the buffers
      NDRegisterAccessor<std::string>::buffer_2D.resize(1);
      NDRegisterAccessor<std::string>::buffer_2D[0].resize(_registerInfo.nElements);

      // The ASCII accessor converts the raw byte stream to strings itself (see doPostRead), so its registration is not
      // used for the payload. It registers a raw-mode consumer with no samples purely so the gate reaches the shared
      // enforcement point: the read coordinator evaluates the gate while it processes the group and records the gate
      // result on this consumer's state, which the accessor reads back through readRegistration().
      registerWithReadCoordinator();

      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAsCooked_impl);
      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(setAsCooked_impl);
    }

    void doReadTransferSynchronously() override { _rawAccessor->readTransfer(); }

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber) override {
      assert(!TransferElement::_isInTransferGroup);
      _rawAccessor->writeTransfer(versionNumber);
      return false;
    }

    void doPostRead(TransferType type, bool hasNewData) override {
      if(!_dev->isOpen()) return; // do not delegate if exception was thrown by us in doPreWrite

      _rawAccessor->setActiveException(this->_activeException);
      _rawAccessor->postRead(type, hasNewData);

      if(!hasNewData) return;

      const size_t nCharsPerElement = _registerInfo.elementPitchBits / 8;
      std::string temp;
      temp.reserve(nCharsPerElement);
      // Turn off linter warning. Yes, we are re-interpreting a byte stream as text.
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      auto* itsrc = reinterpret_cast<char*>(_rawAccessor->begin(_registerInfo.address));
      for(size_t el = 0; el < _registerInfo.nElements; ++el) {
        auto len = strnlen(itsrc, nCharsPerElement);
        temp.resize(len);
        std::strncpy(temp.data(), itsrc, nCharsPerElement);
        itsrc += nCharsPerElement;
        buffer_2D[0][el] = temp;
      }

      // we don't put the setting of the version number into the PrePostActionImplementor
      // because it does not need template specialisation, and the implementer does not
      // know about _versionNumber. It's just easier here.
      this->_versionNumber = _rawAccessor->getVersionNumber();
      // The validity is this consumer's own when it carries a gate (the coordinator marks it faulty while the gate is
      // closed); an ungated accessor keeps taking it from its raw accessor.
      if(_readRegistration.hasGate()) {
        this->_dataValidity = _readRegistration.validity();
      }
      else {
        this->_dataValidity = _rawAccessor->dataValidity();
      }
    }

    void doPreWrite(TransferType type, VersionNumber versionNumber) override {
      if(!_dev->isOpen()) throw ChimeraTK::logic_error("Device not opened.");
      // raw accessor preWrite must be called before our _prePostActionsImplementor.doPreWrite(), as it needs to
      // prepare the buffer in case of unaligned access and acquire the lock.
      _rawAccessor->preWrite(type, versionNumber);

      const size_t nCharsPerElement = _registerInfo.elementPitchBits / 8;
      // Turn off linter warning. Yes, we are re-interpreting a byte stream as text.
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      auto* itdst = reinterpret_cast<char*>(_rawAccessor->begin(_registerInfo.address));
      for(size_t el = 0; el < _registerInfo.nElements; ++el) {
        std::memset(itdst, 0, nCharsPerElement);
        std::strncpy(itdst, buffer_2D[0][el].data(), nCharsPerElement);
        itdst += nCharsPerElement;
      }

      _rawAccessor->setDataValidity(this->_dataValidity);
    }

    void doPreRead(TransferType type) override {
      if(!_dev->isOpen()) throw ChimeraTK::logic_error("Device not opened.");
      _rawAccessor->preRead(type);
    }

    void doPostWrite(TransferType type, VersionNumber versionNumber) override {
      if(!_dev->isOpen()) return; // do not delegate if exception was thrown by us in doPreWrite
      _rawAccessor->setActiveException(this->_activeException);
      _rawAccessor->postWrite(type, versionNumber);
    }

    bool mayReplaceOther(const boost::shared_ptr<TransferElement const>& other) const override {
      auto rhsCasted = boost::dynamic_pointer_cast<const NumericAddressedBackendASCIIAccessor>(other);
      if(rhsCasted.get() == this) {
        return false;
      }
      if(!rhsCasted) return false;
      if(_dev != rhsCasted->_dev) return false;
      if(_registerInfo != rhsCasted->_registerInfo) return false;
      return true;
    }

    bool isReadOnly() const override { return isReadable() && !isWriteable(); }

    bool isReadable() const override { return _registerInfo.isReadable(); }

    bool isWriteable() const override { return _registerInfo.isWriteable(); }

    template<typename COOKED_TYPE>
    COOKED_TYPE getAsCooked_impl(unsigned int, unsigned int) {
      throw ChimeraTK::logic_error("Getting as cooked is only available for raw accessors!");
    }

    template<typename COOKED_TYPE>
    void setAsCooked_impl(unsigned int, unsigned int, COOKED_TYPE) {
      throw ChimeraTK::logic_error("Setting as cooked is only available for raw accessors!");
    }

    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(NumericAddressedBackendASCIIAccessor, getAsCooked_impl, 2);
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(NumericAddressedBackendASCIIAccessor, setAsCooked_impl, 3);

    void setExceptionBackend(boost::shared_ptr<DeviceBackend> exceptionBackend) override {
      this->_exceptionBackend = exceptionBackend;
      _rawAccessor->setExceptionBackend(exceptionBackend);
    }

    /**
     * Get the read consumer's registration with the read coordinator of its raw element. The gate result of this
     * consumer (see Registration::gateClosed()) can therefore be read from outside, e.g. by a GatedByDecorator.
     */
    [[nodiscard]] detail::ReadCoordinator::Registration<std::string>& readRegistration() { return _readRegistration; }

   protected:
    /** Address, size and fixed-point representation information of the register
     * from the map file */
    NumericAddressedRegisterInfo _registerInfo;

    /** raw accessor */
    boost::shared_ptr<NumericAddressedLowLevelTransferElement> _rawAccessor;

    /**
     * The expected value of this consumer's gating register, which opens the gate. Only meaningful when the register
     * carries a gatedBy (see _registerInfo). The gating register accessor itself is owned by the low-level element's
     * gate group, keyed by the gating register path; this accessor does not hold it.
     */
    uint64_t _gateValue{0};

    /**
     * Registration of this read consumer with the read coordinator of its raw element, carrying the gate when the
     * register is gated. It is not used for the payload: the ASCII accessor converts the raw bytes to strings itself.
     */
    detail::ReadCoordinator::Registration<std::string> _readRegistration;

    /** the backend to use for the actual hardware access */
    boost::shared_ptr<NumericAddressedBackend> _dev;

    /** (Re-)register this read consumer with the read coordinator of the current raw element. */
    void registerWithReadCoordinator() {
      const bool hasGate = _registerInfo.channels.front().gatedBy.has_value();
      _readRegistration = detail::ReadCoordinator::Registration<std::string>(&_rawAccessor->_readCoordinator,
          _registerInfo, true, _registerInfo.address - _rawAccessor->_startAddress, _registerInfo.elementPitchBits / 8,
          0, hasGate, _gateValue);
    }

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
      return _rawAccessor->getHardwareAccessingElements();
    }

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override {
      return {_rawAccessor}; // the rawAccessor always returns an empty list
    }

    void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override {
      auto casted = boost::dynamic_pointer_cast<NumericAddressedLowLevelTransferElement>(newElement);
      if(casted && casted->isMergeable(_rawAccessor)) {
        size_t newStartAddress = std::min(casted->_startAddress, _rawAccessor->_startAddress);
        size_t newStopAddress = std::max(
            casted->_startAddress + casted->_numberOfBytes, _rawAccessor->_startAddress + _rawAccessor->_numberOfBytes);
        size_t newNumberOfBytes = newStopAddress - newStartAddress;
        // Unregister before the old element loses its last reference below, then re-register with the merged element,
        // which has a different start address.
        _readRegistration.reset();
        casted->changeAddress(newStartAddress, newNumberOfBytes);
        _rawAccessor = casted;
        registerWithReadCoordinator();
      }
      _rawAccessor->setExceptionBackend(this->_exceptionBackend);
    }

    using NDRegisterAccessor<std::string>::buffer_2D;
  };

} // namespace ChimeraTK
