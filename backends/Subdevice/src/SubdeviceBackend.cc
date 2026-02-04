// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SubdeviceBackend.h"

#include "BackendFactory.h"
#include "Exception.h"
#include "FixedPointConverter.h"
#include "MapFileParser.h"
#include "NDRegisterAccessorDecorator.h"
#include "SubdeviceRegisterAccessor.h"
#include "TransferElement.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <utility>

using namespace std::string_literals;

namespace ChimeraTK {

  boost::shared_ptr<DeviceBackend> SubdeviceBackend::createInstance(
      std::string address, std::map<std::string, std::string> parameters) {
    if(parameters["map"].empty()) {
      throw ChimeraTK::logic_error("Map file name not specified.");
    }

    if(!address.empty()) {
      if(parameters.size() > 1) {
        throw ChimeraTK::logic_error("SubdeviceBackend: You cannot specify both the address string and "
                                     "parameters "
                                     "other than the map file in the device descriptor.");
      }
      // decode target information from the instance
      std::vector<std::string> tokens;
      boost::split(tokens, address, boost::is_any_of(","));
      if(tokens.size() != 3) {
        throw ChimeraTK::logic_error("SubdeviceBackend: There must be exactly 3 "
                                     "parameters in the address string.");
      }
      parameters["type"] = tokens[0];
      parameters["device"] = tokens[1];
      parameters["area"] = tokens[2];
    }

    return boost::shared_ptr<DeviceBackend>(new SubdeviceBackend(parameters));
  }

  /********************************************************************************************************************/

  SubdeviceBackend::SubdeviceBackend(std::map<std::string, std::string> parameters) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);

    // check if type is specified
    if(parameters["type"].empty()) {
      throw ChimeraTK::logic_error("SubdeviceBackend: Type must be specified in the device descriptor.");
    }

    // check if target device is specified and open the target device
    if(parameters["device"].empty()) {
      throw ChimeraTK::logic_error("SubdeviceBackend: Target device name must be "
                                   "specified in the device descriptor.");
    }
    _targetAlias = parameters["device"];

    // type "area":
    if(parameters["type"] == "area") {
      _type = Type::area;
    }
    else if(parameters["type"] == "areaHandshake") {
      _type = Type::areaHandshake;
    }
    // type "3regs":
    else if(parameters["type"] == "3regs") {
      _type = Type::threeRegisters;
    }
    else if(parameters["type"] == "2regs") {
      _type = Type::twoRegisters;

      if(parameters["sleep"].empty()) {
        throw ChimeraTK::logic_error("SubdeviceBackend: Target sleep time must be specified in the device "
                                     "descriptor for type '2regs'.");
      }
    }
    // unknown type
    else {
      throw ChimeraTK::logic_error("SubdeviceBackend: Unknown type '" + parameters["type"] + "' specified.");
    }

    if(needAreaParam()) {
      // check if target register name is specified
      if(parameters["area"].empty()) {
        throw ChimeraTK::logic_error("SubdeviceBackend: Target register name "
                                     "must be specified in the device "
                                     "descriptor for types 'area' and 'areaHandshake'.");
      }
      _targetArea = parameters["area"];
    }
    else {
      // if area is not given, data and address are required
      if(parameters["data"].empty()) {
        throw ChimeraTK::logic_error("SubdeviceBackend: Target data register "
                                     "name must be specified in the device "
                                     "descriptor for types '2regs' and '3regs'.");
      }
      _targetData = parameters["data"];

      // check if all target register names are specified
      if(parameters["address"].empty()) {
        throw ChimeraTK::logic_error("SubdeviceBackend: Target address register "
                                     "name must be specified in the device "
                                     "descriptor for type '2regs' and '3regs'.");
      }
      _targetAddress = parameters["address"];
      // optional parameter for delay between address write and data write
      if(!parameters["dataDelay"].empty()) {
        try {
          _addressToDataDelay = std::stoul(parameters["dataDelay"]);
        }
        catch(std::exception& e) {
          throw ChimeraTK::logic_error("SubdeviceBackend: Invalid value for parameter 'dataDelay': '" +
              parameters["dataDelay"] + "': " + e.what());
        }
      }
    }
    if(needStatusParam()) {
      if(parameters["status"].empty()) {
        throw ChimeraTK::logic_error("SubdeviceBackend: Target status register "
                                     "name must be specified in the device "
                                     "descriptor for types '3regs' and 'areaHandshake'.");
      }
      _targetControl = parameters["status"];
      if(!parameters["timeout"].empty()) {
        try {
          _timeout = std::stoul(parameters["timeout"]);
        }
        catch(std::exception& e) {
          throw ChimeraTK::logic_error(
              "SubdeviceBackend: Invalid value for parameter 'timeout': '" + parameters["timeout"] + "': " + e.what());
        }
      }
    }
    // sleep parameter for 2regs, 3regs or areaHandshake case
    if(!parameters["sleep"].empty()) {
      try {
        _sleepTime = std::stoul(parameters["sleep"]);
      }
      catch(std::exception& e) {
        throw ChimeraTK::logic_error(
            "SubdeviceBackend: Invalid value for parameter 'sleep': '" + parameters["sleep"] + "': " + e.what());
      }
    }
    // parse map file
    if(parameters["map"].empty()) {
      throw ChimeraTK::logic_error("SubdeviceBackend: Map file must be specified.");
    }
    std::tie(_registerMap, _metadataCatalogue) = MapFileParser::parse(parameters["map"]);
    if(_type == Type::twoRegisters || _type == Type::threeRegisters) {
      // FIXME: Turn off readable flag in 2reg/3reg mode
      for(auto info : _registerMap) {
        // we are modifying a copy here
        info.registerAccess = NumericAddressedRegisterInfo::Access::WRITE_ONLY;
        _registerMap.modifyRegister(info); // Should be OK. Should not change the iterator
      }
    }
  }

  /********************************************************************************************************************/

  void SubdeviceBackend::obtainTargetBackend() {
    if(_targetDevice != nullptr) {
      return;
    }
    BackendFactory& factoryInstance = BackendFactory::getInstance();
    _targetDevice = factoryInstance.createBackend(_targetAlias);
  }

  /********************************************************************************************************************/

  void SubdeviceBackend::open() {
    obtainTargetBackend();
    // open target backend, unconditionally as it is also used for recovery
    _targetDevice->open();
    setOpenedAndClearException();
  }

  /********************************************************************************************************************/

  void SubdeviceBackend::close() {
    obtainTargetBackend();
    _targetDevice->close();
    _opened = false;
  }

  /********************************************************************************************************************/

  RegisterCatalogue SubdeviceBackend::getRegisterCatalogue() const {
    return RegisterCatalogue(_registerMap.clone());
  }

  /********************************************************************************************************************/

  MetadataCatalogue SubdeviceBackend::getMetadataCatalogue() const {
    return _metadataCatalogue;
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  class FixedPointConvertingDecorator : public NDRegisterAccessorDecorator<UserType, TargetUserType> {
   public:
    FixedPointConvertingDecorator(const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<TargetUserType>>& target,
        FixedPointConverter<DEPRECATED_FIXEDPOINT_DEFAULT> fixedPointConverter)
    : NDRegisterAccessorDecorator<UserType, TargetUserType>(target),
      _fixedPointConverter(std::move(fixedPointConverter)) {}

    void doPreRead(TransferType type) override { _target->preRead(type); }

    void doPostRead(TransferType type, bool hasNewData) override {
      _target->postRead(type, hasNewData);
      if(!hasNewData) {
        return;
      }
      for(size_t i = 0; i < this->buffer_2D.size(); ++i) {
        _fixedPointConverter.template vectorToCooked<UserType>(
            _target->accessChannel(i).begin(), _target->accessChannel(i).end(), buffer_2D[i].begin());
      }
      this->_dataValidity = _target->dataValidity();
      this->_versionNumber = _target->getVersionNumber();
    }

    void doPreWrite(TransferType type, VersionNumber versionNumber) override {
      for(size_t i = 0; i < this->buffer_2D.size(); ++i) {
        for(size_t j = 0; j < this->buffer_2D[i].size(); ++j) {
          _target->accessChannel(i)[j] = _fixedPointConverter.toRaw<UserType>(buffer_2D[i][j]);
        }
      }
      _target->setDataValidity(this->_dataValidity);
      _target->preWrite(type, versionNumber);
    }

    void doPostWrite(TransferType type, VersionNumber versionNumber) override {
      _target->postWrite(type, versionNumber);
    }

    [[nodiscard]] bool mayReplaceOther(
        const boost::shared_ptr<ChimeraTK::TransferElement const>& other) const override {
      auto casted = boost::dynamic_pointer_cast<FixedPointConvertingDecorator<UserType, TargetUserType> const>(other);
      if(!casted) {
        return false;
      }
      if(_fixedPointConverter != casted->_fixedPointConverter) {
        return false;
      }
      return _target->mayReplaceOther(casted->_target);
    }

   protected:
    FixedPointConverter<DEPRECATED_FIXEDPOINT_DEFAULT> _fixedPointConverter;

    using NDRegisterAccessorDecorator<UserType, TargetUserType>::_target;
    using NDRegisterAccessor<UserType>::buffer_2D;
  };

  /********************************************************************************************************************/

  template<typename TargetUserType>
  class FixedPointConvertingRawDecorator : public NDRegisterAccessorDecorator<TargetUserType> {
   public:
    FixedPointConvertingRawDecorator(const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<TargetUserType>>& target,
        FixedPointConverter<DEPRECATED_FIXEDPOINT_DEFAULT> fixedPointConverter)
    : NDRegisterAccessorDecorator<TargetUserType>(target), _fixedPointConverter(std::move(fixedPointConverter)) {
      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAsCooked_impl);
      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(setAsCooked_impl);
    }

    template<typename COOKED_TYPE>
    // NOLINTNEXTLINE(readability-identifier-naming)
    COOKED_TYPE getAsCooked_impl(unsigned int channel, unsigned int sample) {
      std::vector<int32_t> rawVector(1);
      std::vector<COOKED_TYPE> cookedVector(1);
      rawVector[0] = buffer_2D[channel][sample];
      _fixedPointConverter.template vectorToCooked<COOKED_TYPE>(
          rawVector.begin(), rawVector.end(), cookedVector.begin());
      return cookedVector[0];
    }

    template<typename COOKED_TYPE>
    // NOLINTNEXTLINE(readability-identifier-naming)
    void setAsCooked_impl(unsigned int channel, unsigned int sample, COOKED_TYPE value) {
      buffer_2D[channel][sample] = _fixedPointConverter.toRaw<COOKED_TYPE>(value);
    }

    [[nodiscard]] bool mayReplaceOther(
        const boost::shared_ptr<ChimeraTK::TransferElement const>& other) const override {
      auto casted = boost::dynamic_pointer_cast<FixedPointConvertingRawDecorator<TargetUserType> const>(other);
      if(!casted) {
        return false;
      }
      if(_fixedPointConverter != casted->_fixedPointConverter) {
        return false;
      }
      return _target->mayReplaceOther(casted->_target);
    }

   protected:
    FixedPointConverter<DEPRECATED_FIXEDPOINT_DEFAULT> _fixedPointConverter;

    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(
        FixedPointConvertingRawDecorator<TargetUserType>, getAsCooked_impl, 2);
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(
        FixedPointConvertingRawDecorator<TargetUserType>, setAsCooked_impl, 3);

    using NDRegisterAccessorDecorator<TargetUserType>::_target;
    using NDRegisterAccessor<TargetUserType>::buffer_2D;
  };

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> SubdeviceBackend::getRegisterAccessor_impl(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    obtainTargetBackend();
    boost::shared_ptr<NDRegisterAccessor<UserType>> returnValue;
    if(_type == Type::area) {
      returnValue = getAreaRegisterAccessor<UserType>(registerPathName, numberOfWords, wordOffsetInRegister, flags);
    }
    else if(_type == Type::threeRegisters || _type == Type::twoRegisters || _type == Type::areaHandshake) {
      returnValue =
          getSynchronisedRegisterAccessor<UserType>(registerPathName, numberOfWords, wordOffsetInRegister, flags);
    }
    if(!returnValue) {
      throw ChimeraTK::logic_error("Unknown type");
    }
    returnValue->setExceptionBackend(shared_from_this());

    return returnValue;
  }

  /********************************************************************************************************************/

  void SubdeviceBackend::verifyRegisterAccessorSize(const NumericAddressedRegisterInfo& info, size_t& numberOfWords,
      size_t wordOffsetInRegister, bool forceAlignment) {
    // check that the bar is 0
    if(info.bar != 0) {
      //       throw ChimeraTK::logic_error("SubdeviceBackend: BARs other then 0 are not supported. Register '" +
      //           registerPathName + "' is in BAR " + std::to_string(info->bar) + ".");
      std::cout << "SubdeviceBackend: WARNING: BAR others then 0 detected. BAR 0 will be used instead. Register "
                << info.pathName << " is in BAR " << std::to_string(info.bar) << "." << std::endl;
    }

    // check that the register is not a 2D multiplexed register, which is not yet
    // supported
    if(info.channels.size() != 1) {
      throw ChimeraTK::logic_error("SubdeviceBackend: 2D multiplexed registers are not yet supported.");
    }

    // Partial accessors are not implemented correctly yet. Better throw than getting something that seems to work but
    // does the wrong thing.
    if(_type == Type::threeRegisters || _type == Type::twoRegisters) {
      if(wordOffsetInRegister != 0) {
        throw ChimeraTK::logic_error("SubdeviceBackend: Partial accessors are not supported yet for type "
                                     "threeRegisters and twoRegisters. Register " +
            info.pathName + " has requested offset " + std::to_string(wordOffsetInRegister));
      }
      if((numberOfWords != 0) && (numberOfWords != info.nElements)) {
        throw ChimeraTK::logic_error("SubdeviceBackend: Partial accessors are not supported yet for type "
                                     "threeRegisters and twoRegisters. Register " +
            info.pathName + " has requested nElements " + std::to_string(numberOfWords) + " of " +
            std::to_string(info.nElements));
      }
    }

    // compute full offset (from map file and function arguments)
    size_t byteOffset = info.address + sizeof(int32_t) * wordOffsetInRegister;
    if(forceAlignment && (byteOffset % 4 != 0)) {
      throw ChimeraTK::logic_error("SubdeviceBackend: Only addresses which are a "
                                   "multiple of 4 are supported.");
    }

    // compute effective length
    if(numberOfWords == 0) {
      numberOfWords = info.nElements;
    }
    else if(numberOfWords > info.nElements) {
      throw ChimeraTK::logic_error("SubdeviceBackend: Requested " + std::to_string(numberOfWords) +
          " elements from register '" + info.pathName + "', which only has a length of " +
          std::to_string(info.nElements) + " elements.");
    }

    // Check that the requested register fits in the register description. The downstream register might be larger
    // so we cannot delegate the check
    if(numberOfWords + wordOffsetInRegister > info.nElements) {
      throw ChimeraTK::logic_error(
          "SubdeviceBackend: Requested offset + number of words exceeds the size of the register '" + info.pathName +
          "'!");
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> SubdeviceBackend::getAreaRegisterAccessor(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    assert(_type == Type::area);

    // obtain register info
    auto info = _registerMap.getBackendRegister(registerPathName);
    verifyRegisterAccessorSize(info, numberOfWords, wordOffsetInRegister, true);

    // store raw flag for later (since we modify the flags)
    bool isRaw = flags.has(AccessMode::raw);

    // obtain target accessor in raw mode
    size_t wordOffset = (info.address + sizeof(int32_t) * wordOffsetInRegister) / 4;
    flags.add(AccessMode::raw);
    auto rawAcc = _targetDevice->getRegisterAccessor<int32_t>(_targetArea, numberOfWords, wordOffset, flags);

    // decorate with appropriate FixedPointConvertingDecorator. This is done even
    // when in raw mode so we can properly implement getAsCooked()/setAsCooked().
    if(!isRaw) {
      return boost::make_shared<FixedPointConvertingDecorator<UserType, int32_t>>(rawAcc,
          FixedPointConverter<DEPRECATED_FIXEDPOINT_DEFAULT>(registerPathName, info.channels.front().width,
              info.channels.front().nFractionalBits, info.channels.front().signedFlag));
    }
    // this is handled by the template specialisation for int32_t
    throw ChimeraTK::logic_error("Given UserType when obtaining the SubdeviceBackend in raw mode does not "s +
        "match the expected type. Use an int32_t instead! (Register name: " + registerPathName + "')");
  }

  /********************************************************************************************************************/

  boost::shared_ptr<SubdeviceRegisterAccessor> SubdeviceBackend::accessorCreationHelper(
      const NumericAddressedRegisterInfo& info, size_t numberOfWords, size_t wordOffsetInRegister,
      AccessModeFlags flags) {
    flags.checkForUnknownFlags({AccessMode::raw});

    verifyRegisterAccessorSize(info, numberOfWords, wordOffsetInRegister, false);

    // check if register access properly specified in map file
    if(!info.isWriteable()) {
      throw ChimeraTK::logic_error("SubdeviceBackend: Subdevices of type 3reg or "
                                   "2reg or areaHandshake must have writeable registers only!");
    }

    // obtain target accessors
    boost::shared_ptr<NDRegisterAccessor<int32_t>> accAddress, accData;
    if(!needAreaParam()) {
      accAddress = _targetDevice->getRegisterAccessor<int32_t>(_targetAddress, 1, 0, {});
      accData = _targetDevice->getRegisterAccessor<int32_t>(_targetData, 0, 0, {});
    }
    else {
      // check alignment just like it is done in 'area' type subdevice which is based on raw int32 accessors to target
      verifyRegisterAccessorSize(info, numberOfWords, wordOffsetInRegister, true);

      // obtain target accessor in raw mode
      size_t wordOffset = (info.address + sizeof(int32_t) * wordOffsetInRegister) / 4;
      flags.add(AccessMode::raw);
      accData = _targetDevice->getRegisterAccessor<int32_t>(_targetArea, numberOfWords, wordOffset, flags);
    }
    boost::shared_ptr<NDRegisterAccessor<int32_t>> accStatus;
    if(needStatusParam()) {
      accStatus = _targetDevice->getRegisterAccessor<int32_t>(_targetControl, 1, 0, {});
    }

    size_t byteOffset = info.address + sizeof(int32_t) * wordOffsetInRegister;
    auto sharedThis = boost::enable_shared_from_this<DeviceBackend>::shared_from_this();

    return boost::make_shared<SubdeviceRegisterAccessor>(boost::dynamic_pointer_cast<SubdeviceBackend>(sharedThis),
        info.pathName, accAddress, accData, accStatus, byteOffset, numberOfWords);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> SubdeviceBackend::getSynchronisedRegisterAccessor(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister,
      const AccessModeFlags& flags) {
    auto info = _registerMap.getBackendRegister(registerPathName);
    boost::shared_ptr<SubdeviceRegisterAccessor> rawAcc =
        accessorCreationHelper(info, numberOfWords, wordOffsetInRegister, flags);

    // decorate with appropriate FixedPointConvertingDecorator. This is done even
    // when in raw mode so we can properly implement getAsCooked()/setAsCooked().
    if(!flags.has(AccessMode::raw)) {
      return boost::make_shared<FixedPointConvertingDecorator<UserType, int32_t>>(rawAcc,
          FixedPointConverter<DEPRECATED_FIXEDPOINT_DEFAULT>(registerPathName, info.channels.front().width,
              info.channels.front().nFractionalBits, info.channels.front().signedFlag));
    }
    // this is handled by the template specialisation for int32_t
    throw ChimeraTK::logic_error("Given UserType when obtaining the SubdeviceBackend in raw mode does not "s +
        "match the expected type. Use an int32_t instead! (Register name: " + registerPathName + "')");
  }

  /********************************************************************************************************************/

  template<>
  boost::shared_ptr<NDRegisterAccessor<int32_t>> SubdeviceBackend::getAreaRegisterAccessor<int32_t>(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    assert(_type == Type::area);

    // obtain register info
    auto info = _registerMap.getBackendRegister(registerPathName);
    verifyRegisterAccessorSize(info, numberOfWords, wordOffsetInRegister, true);

    // store raw flag for later (since we modify the flags)
    bool isRaw = flags.has(AccessMode::raw);

    // obtain target accessor in raw mode
    size_t wordOffset = (info.address + sizeof(int32_t) * wordOffsetInRegister) / 4;
    flags.add(AccessMode::raw);
    auto rawAcc = _targetDevice->getRegisterAccessor<int32_t>(_targetArea, numberOfWords, wordOffset, flags);

    // decorate with appropriate FixedPointConvertingDecorator. This is done even
    // when in raw mode so we can properly implement getAsCooked()/setAsCooked().
    if(!isRaw) {
      return boost::make_shared<FixedPointConvertingDecorator<int32_t, int32_t>>(rawAcc,
          FixedPointConverter<DEPRECATED_FIXEDPOINT_DEFAULT>(registerPathName, info.channels.front().width,
              info.channels.front().nFractionalBits, info.channels.front().signedFlag));
    }
    return boost::make_shared<FixedPointConvertingRawDecorator<int32_t>>(rawAcc,
        FixedPointConverter<DEPRECATED_FIXEDPOINT_DEFAULT>(registerPathName, info.channels.front().width,
            info.channels.front().nFractionalBits, info.channels.front().signedFlag));
  }

  /********************************************************************************************************************/

  template<>
  boost::shared_ptr<NDRegisterAccessor<int32_t>> SubdeviceBackend::getSynchronisedRegisterAccessor<int32_t>(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister,
      const AccessModeFlags& flags) {
    auto info = _registerMap.getBackendRegister(registerPathName);
    boost::shared_ptr<SubdeviceRegisterAccessor> rawAcc =
        accessorCreationHelper(info, numberOfWords, wordOffsetInRegister, flags);

    // decorate with appropriate FixedPointConvertingDecorator. This is done even
    // when in raw mode so we can properly implement getAsCooked()/setAsCooked().
    if(!flags.has(AccessMode::raw)) {
      return boost::make_shared<FixedPointConvertingDecorator<int32_t, int32_t>>(rawAcc,
          FixedPointConverter<DEPRECATED_FIXEDPOINT_DEFAULT>(registerPathName, info.channels.front().width,
              info.channels.front().nFractionalBits, info.channels.front().signedFlag));
    }
    return boost::make_shared<FixedPointConvertingRawDecorator<int32_t>>(rawAcc,
        FixedPointConverter<DEPRECATED_FIXEDPOINT_DEFAULT>(registerPathName, info.channels.front().width,
            info.channels.front().nFractionalBits, info.channels.front().signedFlag));
  }

  /********************************************************************************************************************/

  void SubdeviceBackend::setExceptionImpl() noexcept {
    obtainTargetBackend();
    _targetDevice->setException(getActiveExceptionMessage());
  }

  /********************************************************************************************************************/

  void SubdeviceBackend::activateAsyncRead() noexcept {
    obtainTargetBackend();
    _targetDevice->activateAsyncRead();
  }
  /********************************************************************************************************************/

  std::set<DeviceBackend::BackendID> SubdeviceBackend::getInvolvedBackendIDs() {
    obtainTargetBackend();
    std::set<DeviceBackend::BackendID> retVal{getBackendID()};
    retVal.merge(_targetDevice->getInvolvedBackendIDs());
    return retVal;
  }

} // namespace ChimeraTK
