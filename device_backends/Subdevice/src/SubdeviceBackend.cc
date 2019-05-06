#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "SubdeviceBackend.h"

#include "BackendFactory.h"
#include "Exception.h"
#include "FixedPointConverter.h"
#include "MapFileParser.h"
#include "NDRegisterAccessorDecorator.h"
#include "SubdeviceRegisterAccessor.h"

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

  /*******************************************************************************************************************/

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
    targetAlias = parameters["device"];

    // type "area":
    if(parameters["type"] == "area") {
      type = Type::area;

      // check if target register name is specified
      if(parameters["area"].empty()) {
        throw ChimeraTK::logic_error("SubdeviceBackend: Target register name "
                                     "must be specified in the device "
                                     "descriptor for type 'area'.");
      }
      targetArea = parameters["area"];
    }
    // type "3regs":
    else if(parameters["type"] == "3regs") {
      type = Type::threeRegisters;

      // check if all target register names are specified
      if(parameters["address"].empty()) {
        throw ChimeraTK::logic_error("SubdeviceBackend: Target address register "
                                     "name must be specified in the device "
                                     "descriptor for type '3regs'.");
      }
      if(parameters["data"].empty()) {
        throw ChimeraTK::logic_error("SubdeviceBackend: Target data register "
                                     "name must be specified in the device "
                                     "descriptor for type '3regs'.");
      }
      if(parameters["status"].empty()) {
        throw ChimeraTK::logic_error("SubdeviceBackend: Target status register "
                                     "name must be specified in the device "
                                     "descriptor for type '3regs'.");
      }
      targetAddress = parameters["address"];
      targetData = parameters["data"];
      targetControl = parameters["status"];
      if(!parameters["sleep"].empty()) {
        try {
          sleepTime = std::stoi(parameters["sleep"]);
        }
        catch(std::exception& e) {
          throw ChimeraTK::logic_error(
              "SubdeviceBackend: Invalid value for parameter 'sleep': '" + parameters["sleep"] + "': " + e.what());
        }
      }
    }
    else if(parameters["type"] == "2regs") {
      type = Type::twoRegisters;

      // check if all target register names are specified
      if(parameters["address"].empty()) {
        throw ChimeraTK::logic_error("SubdeviceBackend: Target address register "
                                     "name must be specified in the device "
                                     "descriptor for type '2regs'.");
      }
      if(parameters["data"].empty()) {
        throw ChimeraTK::logic_error("SubdeviceBackend: Target data register "
                                     "name must be specified in the device "
                                     "descriptor for type '2regs'.");
      }
      if(parameters["sleep"].empty()) {
        throw ChimeraTK::logic_error("SubdeviceBackend: Target sleep time must be specified in the device "
                                     "descriptor for type '2regs'.");
      }
      targetAddress = parameters["address"];
      targetData = parameters["data"];
      try {
        sleepTime = std::stoi(parameters["sleep"]);
      }
      catch(std::exception& e) {
        throw ChimeraTK::logic_error(
            "SubdeviceBackend: Invalid value for parameter 'sleep': '" + parameters["sleep"] + "': " + e.what());
      }
    }
    // unknown type
    else {
      throw ChimeraTK::logic_error("SubdeviceBackend: Unknown type '" + parameters["type"] + "' specified.");
    }

    // parse map file
    if(parameters["map"].empty()) {
      throw ChimeraTK::logic_error("SubdeviceBackend: Map file must be specified.");
    }
    MapFileParser parser;
    _registerMap = parser.parse(parameters["map"]);
    _catalogue = _registerMap->getRegisterCatalogue();
  }

  /*******************************************************************************************************************/

  void SubdeviceBackend::obtainTargetBackend() {
    if(targetDevice != nullptr) return;
    BackendFactory& factoryInstance = BackendFactory::getInstance();
    targetDevice = factoryInstance.createBackend(targetAlias);
  }

  /*******************************************************************************************************************/

  void SubdeviceBackend::open() {
    obtainTargetBackend();
    // open target backend
    if(!targetDevice->isOpen()) { // createBackend may return an already opened
                                  // instance for some backends
      targetDevice->open();
    }
    _opened = true;
  }

  /*******************************************************************************************************************/

  void SubdeviceBackend::close() {
    obtainTargetBackend();
    targetDevice->close();
    _opened = false;
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  class FixedPointConvertingDecorator : public NDRegisterAccessorDecorator<UserType, TargetUserType> {
   public:
    FixedPointConvertingDecorator(const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<TargetUserType>>& target,
        FixedPointConverter fixedPointConverter)
    : NDRegisterAccessorDecorator<UserType, TargetUserType>(target), _fixedPointConverter(fixedPointConverter) {}

    void doPreRead() override { _target->preRead(); }

    void doPostRead() override {
      _target->postRead();
      for(size_t i = 0; i < this->buffer_2D.size(); ++i) {
        _fixedPointConverter.template vectorToCooked<UserType>(
            _target->accessChannel(i).begin(), _target->accessChannel(i).end(), buffer_2D[i].begin());
      }
    }

    void doPreWrite() override {
      for(size_t i = 0; i < this->buffer_2D.size(); ++i) {
        for(size_t j = 0; j < this->buffer_2D[i].size(); ++j) {
          _target->accessChannel(i)[j] = _fixedPointConverter.toRaw<UserType>(buffer_2D[i][j]);
        }
      }
      _target->preWrite();
    }

    void doPostWrite() override { _target->postWrite(); }

    bool mayReplaceOther(const boost::shared_ptr<ChimeraTK::TransferElement const>& other) const override {
      auto casted = boost::dynamic_pointer_cast<FixedPointConvertingDecorator<UserType, TargetUserType> const>(other);
      if(!casted) return false;
      if(_fixedPointConverter != casted->_fixedPointConverter) return false;
      return _target->mayReplaceOther(casted->_target);
    }

   protected:
    FixedPointConverter _fixedPointConverter;

    using NDRegisterAccessorDecorator<UserType, TargetUserType>::_target;
    using NDRegisterAccessor<UserType>::buffer_2D;
  };

  /********************************************************************************************************************/

  template<typename TargetUserType>
  class FixedPointConvertingRawDecorator : public NDRegisterAccessorDecorator<TargetUserType> {
   public:
    FixedPointConvertingRawDecorator(const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<TargetUserType>>& target,
        FixedPointConverter fixedPointConverter)
    : NDRegisterAccessorDecorator<TargetUserType>(target), _fixedPointConverter(fixedPointConverter) {
      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAsCooked_impl);
      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(setAsCooked_impl);
    }

    template<typename COOKED_TYPE>
    COOKED_TYPE getAsCooked_impl(unsigned int channel, unsigned int sample) {
      std::vector<int32_t> rawVector(1);
      std::vector<COOKED_TYPE> cookedVector(1);
      rawVector[0] = buffer_2D[channel][sample];
      _fixedPointConverter.template vectorToCooked<COOKED_TYPE>(
          rawVector.begin(), rawVector.end(), cookedVector.begin());
      return cookedVector[0];
    }

    template<typename COOKED_TYPE>
    void setAsCooked_impl(unsigned int channel, unsigned int sample, COOKED_TYPE value) {
      buffer_2D[channel][sample] = _fixedPointConverter.toRaw<COOKED_TYPE>(value);
    }

    bool mayReplaceOther(const boost::shared_ptr<ChimeraTK::TransferElement const>& other) const override {
      auto casted = boost::dynamic_pointer_cast<FixedPointConvertingRawDecorator<TargetUserType> const>(other);
      if(!casted) return false;
      if(_fixedPointConverter != casted->_fixedPointConverter) return false;
      return _target->mayReplaceOther(casted->_target);
    }

   protected:
    FixedPointConverter _fixedPointConverter;

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
    if(type == Type::area) {
      return getRegisterAccessor_area<UserType>(registerPathName, numberOfWords, wordOffsetInRegister, flags);
    }
    else if(type == Type::threeRegisters || type == Type::twoRegisters) {
      return getRegisterAccessor_3regs<UserType>(registerPathName, numberOfWords, wordOffsetInRegister, flags);
    }
    throw ChimeraTK::logic_error("Unknown type");
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> SubdeviceBackend::getRegisterAccessor_area(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    assert(type == Type::area);

    // obtain register info
    auto info = boost::static_pointer_cast<RegisterInfoMap::RegisterInfo>(_catalogue.getRegister(registerPathName));

    // check that the bar is 0
    if(info->bar != 0) {
      throw ChimeraTK::logic_error("SubdeviceBackend: BARs other then 0 are not supported. Register '" +
          registerPathName + "' is in BAR " + std::to_string(info->bar) + ".");
    }

    // check that the register is not a 2D multiplexed register, which is not yet
    // supported
    if(info->is2DMultiplexed) {
      throw ChimeraTK::logic_error("SubdeviceBackend: 2D multiplexed registers are not yet supported.");
    }

    // compute full offset (from map file and function arguments)
    size_t byteOffset = info->address + sizeof(int32_t) * wordOffsetInRegister;
    if(byteOffset % 4 != 0) {
      throw ChimeraTK::logic_error("SubdeviceBackend: Only addresses which are a "
                                   "multiple of 4 are supported.");
    }
    size_t wordOffset = byteOffset / 4;

    // compute effective length
    if(numberOfWords == 0) {
      numberOfWords = info->nElements;
    }
    else if(numberOfWords > info->nElements) {
      throw ChimeraTK::logic_error("SubdeviceBackend: Requested " + std::to_string(numberOfWords) +
          " elements from register '" + registerPathName + "', which only has a length of " +
          std::to_string(info->nElements) + " elements.");
    }

    // check if raw transfer?
    bool isRaw = flags.has(AccessMode::raw);

    // obtain target accessor in raw mode
    flags.add(AccessMode::raw);
    auto rawAcc = targetDevice->getRegisterAccessor<int32_t>(targetArea, numberOfWords, wordOffset, flags);

    // decorate with appropriate FixedPointConvertingDecorator. This is done even
    // when in raw mode so we can properly implement getAsCooked()/setAsCooked().
    if(!isRaw) {
      return boost::make_shared<FixedPointConvertingDecorator<UserType, int32_t>>(
          rawAcc, FixedPointConverter(registerPathName, info->width, info->nFractionalBits, info->signedFlag));
    }
    else {
      // this is handled by the template specialisation for int32_t
      throw ChimeraTK::logic_error("Given UserType when obtaining the SubdeviceBackend in raw mode does "
                                   "not "
                                   "match the expected type. Use an int32_t instead! (Register name: " +
          registerPathName + "')");
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> SubdeviceBackend::getRegisterAccessor_3regs(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    assert(type == Type::threeRegisters || type == Type::twoRegisters);
    flags.checkForUnknownFlags({AccessMode::raw});

    // obtain register info
    auto info = boost::static_pointer_cast<RegisterInfoMap::RegisterInfo>(_catalogue.getRegister(registerPathName));

    // check that the bar is 0
    if(info->bar != 0) {
      throw ChimeraTK::logic_error("SubdeviceBackend: BARs other then 0 are not supported. Register '" +
          registerPathName + "' is in BAR " + std::to_string(info->bar) + ".");
    }

    // check that the register is not a 2D multiplexed register, which is not yet
    // supported
    if(info->is2DMultiplexed) {
      throw ChimeraTK::logic_error("SubdeviceBackend: 2D multiplexed registers are not yet supported.");
    }

    // compute full offset (from map file and function arguments)
    size_t byteOffset = info->address + sizeof(int32_t) * wordOffsetInRegister;

    // compute effective length
    if(numberOfWords == 0) {
      numberOfWords = info->nElements;
    }
    else if(numberOfWords > info->nElements) {
      throw ChimeraTK::logic_error("SubdeviceBackend: Requested " + std::to_string(numberOfWords) +
          " elements from register '" + registerPathName + "', which only has a length of " +
          std::to_string(info->nElements) + " elements.");
    }

    // check if register access properly specified in map file
    if(!info->isWriteable()) {
      throw ChimeraTK::logic_error("SubdeviceBackend: Subdevices of type 3reg or "
                                   "2reg must have writeable registers only!");
    }

    // check if raw transfer?
    bool isRaw = flags.has(AccessMode::raw);

    // obtain target accessor in raw mode
    auto accAddress = targetDevice->getRegisterAccessor<int32_t>(targetAddress, 1, 0, {AccessMode::raw});
    auto accData = targetDevice->getRegisterAccessor<int32_t>(targetData, 1, 0, {AccessMode::raw});
    boost::shared_ptr<NDRegisterAccessor<int32_t>> accStatus;
    if(type == Type::threeRegisters) {
      accStatus = targetDevice->getRegisterAccessor<int32_t>(targetControl, 1, 0, {AccessMode::raw});
    }
    auto sharedThis = boost::enable_shared_from_this<DeviceBackend>::shared_from_this();
    auto rawAcc =
        boost::make_shared<SubdeviceRegisterAccessor>(boost::dynamic_pointer_cast<SubdeviceBackend>(sharedThis),
            registerPathName, accAddress, accData, accStatus, byteOffset, numberOfWords);

    // decorate with appropriate FixedPointConvertingDecorator. This is done even
    // when in raw mode so we can properly implement getAsCooked()/setAsCooked().
    if(!isRaw) {
      return boost::make_shared<FixedPointConvertingDecorator<UserType, int32_t>>(
          rawAcc, FixedPointConverter(registerPathName, info->width, info->nFractionalBits, info->signedFlag));
    }
    else {
      // this is handled by the template specialisation for int32_t
      throw ChimeraTK::logic_error("Given UserType when obtaining the SubdeviceBackend in raw mode does "
                                   "not "
                                   "match the expected type. Use an int32_t instead! (Register name: " +
          registerPathName + "')");
    }
  }

  /********************************************************************************************************************/

  template<>
  boost::shared_ptr<NDRegisterAccessor<int32_t>> SubdeviceBackend::getRegisterAccessor_area<int32_t>(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    assert(type == Type::area);

    // obtain register info
    auto info = boost::static_pointer_cast<RegisterInfoMap::RegisterInfo>(_catalogue.getRegister(registerPathName));

    // check that the bar is 0
    if(info->bar != 0) {
      throw ChimeraTK::logic_error("SubdeviceBackend: BARs other then 0 are not supported. Register '" +
          registerPathName + "' is in BAR " + std::to_string(info->bar) + ".");
    }

    // check that the register is not a 2D multiplexed register, which is not yet
    // supported
    if(info->is2DMultiplexed) {
      throw ChimeraTK::logic_error("SubdeviceBackend: 2D multiplexed registers are not yet supported.");
    }

    // compute full offset (from map file and function arguments)
    size_t byteOffset = info->address + sizeof(int32_t) * wordOffsetInRegister;
    if(byteOffset % 4 != 0) {
      throw ChimeraTK::logic_error("SubdeviceBackend: Only addresses which are a "
                                   "multiple of 4 are supported.");
    }
    size_t wordOffset = byteOffset / 4;

    // compute effective length
    if(numberOfWords == 0) {
      numberOfWords = info->nElements;
    }
    else if(numberOfWords > info->nElements) {
      throw ChimeraTK::logic_error("SubdeviceBackend: Requested " + std::to_string(numberOfWords) +
          " elements from register '" + registerPathName + "', which only has a length of " +
          std::to_string(info->nElements) + " elements.");
    }

    // check if raw transfer?
    bool isRaw = flags.has(AccessMode::raw);

    // obtain target accessor in raw mode
    flags.add(AccessMode::raw);
    auto rawAcc = targetDevice->getRegisterAccessor<int32_t>(targetArea, numberOfWords, wordOffset, flags);

    // decorate with appropriate FixedPointConvertingDecorator. This is done even
    // when in raw mode so we can properly implement getAsCooked()/setAsCooked().
    if(!isRaw) {
      return boost::make_shared<FixedPointConvertingDecorator<int32_t, int32_t>>(
          rawAcc, FixedPointConverter(registerPathName, info->width, info->nFractionalBits, info->signedFlag));
    }
    else {
      return boost::make_shared<FixedPointConvertingRawDecorator<int32_t>>(
          rawAcc, FixedPointConverter(registerPathName, info->width, info->nFractionalBits, info->signedFlag));
    }
  }

  /********************************************************************************************************************/

  template<>
  boost::shared_ptr<NDRegisterAccessor<int32_t>> SubdeviceBackend::getRegisterAccessor_3regs<int32_t>(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    assert(type == Type::threeRegisters || type == Type::twoRegisters);
    flags.checkForUnknownFlags({AccessMode::raw});

    // obtain register info
    auto info = boost::static_pointer_cast<RegisterInfoMap::RegisterInfo>(_catalogue.getRegister(registerPathName));

    // check that the bar is 0
    if(info->bar != 0) {
      throw ChimeraTK::logic_error("SubdeviceBackend: BARs other then 0 are not supported. Register '" +
          registerPathName + "' is in BAR " + std::to_string(info->bar) + ".");
    }

    // check that the register is not a 2D multiplexed register, which is not yet
    // supported
    if(info->is2DMultiplexed) {
      throw ChimeraTK::logic_error("SubdeviceBackend: 2D multiplexed registers are not yet supported.");
    }

    // compute full offset (from map file and function arguments)
    size_t byteOffset = info->address + sizeof(int32_t) * wordOffsetInRegister;

    // compute effective length
    if(numberOfWords == 0) {
      numberOfWords = info->nElements;
    }
    else if(numberOfWords > info->nElements) {
      throw ChimeraTK::logic_error("SubdeviceBackend: Requested " + std::to_string(numberOfWords) +
          " elements from register '" + registerPathName + "', which only has a length of " +
          std::to_string(info->nElements) + " elements.");
    }

    // check if register access properly specified in map file
    if(!info->isWriteable()) {
      throw ChimeraTK::logic_error("SubdeviceBackend: Subdevices of type 3reg or "
                                   "2reg must have writeable registers only!");
    }

    // check if raw transfer?
    bool isRaw = flags.has(AccessMode::raw);

    // obtain target accessor in raw mode
    auto accAddress = targetDevice->getRegisterAccessor<int32_t>(targetAddress, 1, 0, {AccessMode::raw});
    auto accData = targetDevice->getRegisterAccessor<int32_t>(targetData, 1, 0, {AccessMode::raw});
    boost::shared_ptr<NDRegisterAccessor<int32_t>> accStatus;
    if(type == Type::threeRegisters) {
      accStatus = targetDevice->getRegisterAccessor<int32_t>(targetControl, 1, 0, {AccessMode::raw});
    }
    auto sharedThis = boost::enable_shared_from_this<DeviceBackend>::shared_from_this();
    auto rawAcc =
        boost::make_shared<SubdeviceRegisterAccessor>(boost::dynamic_pointer_cast<SubdeviceBackend>(sharedThis),
            registerPathName, accAddress, accData, accStatus, byteOffset, numberOfWords);

    // decorate with appropriate FixedPointConvertingDecorator. This is done even
    // when in raw mode so we can properly implement getAsCooked()/setAsCooked().
    if(!isRaw) {
      return boost::make_shared<FixedPointConvertingDecorator<int32_t, int32_t>>(
          rawAcc, FixedPointConverter(registerPathName, info->width, info->nFractionalBits, info->signedFlag));
    }
    else {
      return boost::make_shared<FixedPointConvertingRawDecorator<int32_t>>(
          rawAcc, FixedPointConverter(registerPathName, info->width, info->nFractionalBits, info->signedFlag));
    }
  }

} // namespace ChimeraTK
