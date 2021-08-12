/*
 * AddressBasedBackend.cc
 *
 *  Created on: Feb 1, 2016
 *      Author: Martin Hierholzer
 */

#include "NumericAddressedBackend.h"
#include "Exception.h"
#include "MapFileParser.h"
#include "NumericAddress.h"
#include "NumericAddressedBackendMuxedRegisterAccessor.h"
#include "NumericAddressedBackendRegisterAccessor.h"
#include "AsyncNDRegisterAccessor.h"
#include "NumericAddressedInterruptDispatcher.h"

namespace ChimeraTK {

  NumericAddressedBackend::NumericAddressedBackend(std::string mapFileName) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
    if(mapFileName != "") {
      MapFileParser parser;
      _registerMap = parser.parse(mapFileName);
      _catalogue = _registerMap->getRegisterCatalogue();

      // create all the interrupt dispatchers that are described in the map file
      for(auto& interruptController : _registerMap->getListOfInterrupts()) {
        // interruptController is a pair<int, set<int>>, containing the controller number and a set of associated interrupts
        for(auto interruptNumber : interruptController.second) {
          _interruptDispatchers[{interruptController.first, interruptNumber}] =
              boost::make_shared<NumericAddressedInterruptDispatcher>();
        }
      }
    }
    else {
      _registerMap = boost::shared_ptr<RegisterInfoMap>();
    }
  }

  /********************************************************************************************************************/

  boost::shared_ptr<RegisterInfoMap::RegisterInfo> NumericAddressedBackend::getRegisterInfo(
      const RegisterPath& registerPathName) {
    if(!registerPathName.startsWith(numeric_address::BAR)) {
      boost::shared_ptr<RegisterInfo> info = _catalogue.getRegister(registerPathName);
      return boost::static_pointer_cast<RegisterInfoMap::RegisterInfo>(info);
    }
    else {
      auto components = registerPathName.getComponents();
      if(components.size() != 3) {
        throw ChimeraTK::logic_error("Illegal numeric address: '" + (registerPathName) + "'");
      }
      auto bar = std::stoi(components[1]);
      size_t pos = components[2].find_first_of("*");
      auto address = std::stoi(components[2].substr(0, pos));
      size_t nBytes;
      if(pos != std::string::npos) {
        nBytes = std::stoi(components[2].substr(pos + 1));
      }
      else {
        nBytes = sizeof(int32_t);
      }
      auto nElements = nBytes / sizeof(int32_t);
      if(nBytes == 0 || nBytes % sizeof(int32_t) != 0) {
        throw ChimeraTK::logic_error("Illegal numeric address: '" + (registerPathName) + "'");
      }
      return boost::make_shared<RegisterInfoMap::RegisterInfo>(registerPathName, nElements, address, nBytes, bar);
    }
  }

  /* Throw exception if called directly and not implemented by backend */
  void NumericAddressedBackend::read([[maybe_unused]] uint8_t bar, [[maybe_unused]] uint32_t address,
      [[maybe_unused]] int32_t* data, [[maybe_unused]] size_t sizeInBytes) {
    throw ChimeraTK::logic_error("NumericAddressedBackend: internal error: interface read() called w/ 32bit address");
  }

  void NumericAddressedBackend::write([[maybe_unused]] uint8_t bar, [[maybe_unused]] uint32_t address,
      [[maybe_unused]] int32_t const* data, [[maybe_unused]] size_t sizeInBytes) {
    throw ChimeraTK::logic_error("NumericAddressedBackend: internal error: interface write() called w/ 32bit address");
  }

  /* Call 32-bit address implementation by default, for backends that don't implement 64-bit */
  void NumericAddressedBackend::read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) {
    read(static_cast<uint8_t>(bar), static_cast<uint32_t>(address), data, sizeInBytes);
  }

  void NumericAddressedBackend::write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) {
    write(static_cast<uint8_t>(bar), static_cast<uint32_t>(address), data, sizeInBytes);
  }

  // Default range of valid BARs
  bool NumericAddressedBackend::barIndexValid(uint64_t bar) { return bar <= 5 || bar == 13; }

  /********************************************************************************************************************/

  boost::shared_ptr<const RegisterInfoMap> NumericAddressedBackend::getRegisterMap() const { return _registerMap; }

  /********************************************************************************************************************/

  void NumericAddressedBackend::checkRegister(const std::string& regName, const std::string& regModule, size_t dataSize,
      uint32_t addRegOffset, uint32_t& retDataSize, uint32_t& retRegOff, uint8_t& retRegBar) const {
    RegisterInfoMap::RegisterInfo registerInfo;
    _registerMap->getRegisterInfo(regName, registerInfo, regModule);
    if(addRegOffset % 4) {
      throw ChimeraTK::logic_error("Register offset must be divisible by 4");
    }
    if(dataSize) {
      if(dataSize % 4) {
        throw ChimeraTK::logic_error("Data size must be divisible by 4");
      }
      if(dataSize > registerInfo.nBytes - addRegOffset) {
        throw ChimeraTK::logic_error("Data size exceed register size");
      }
      retDataSize = dataSize;
    }
    else {
      retDataSize = registerInfo.nBytes;
    }
    retRegBar = registerInfo.bar;
    retRegOff = registerInfo.address + addRegOffset;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> NumericAddressedBackend::getRegisterAccessor_impl(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    if(flags.has(AccessMode::wait_for_new_data)) {
      // get the interrupt information from the map file
      boost::shared_ptr<RegisterInfo> info = getRegisterInfo(registerPathName);
      auto registerInfo = boost::static_pointer_cast<RegisterInfoMap::RegisterInfo>(info);
      if(!registerInfo->getSupportedAccessModes().has(AccessMode::wait_for_new_data)) {
        throw ChimeraTK::logic_error(
            "Register " + registerPathName + " does not support AccessMode::wait_for_new_data.");
      }

      auto interruptDispatcher =
          _interruptDispatchers[{registerInfo->interruptCtrlNumber, registerInfo->interruptNumber}];
      assert(interruptDispatcher);
      auto newSubscriber = interruptDispatcher->subscribe<UserType>(
          boost::dynamic_pointer_cast<NumericAddressedBackend>(shared_from_this()), registerPathName, numberOfWords,
          wordOffsetInRegister, flags);
      startInterruptHandlingThread(registerInfo->interruptCtrlNumber, registerInfo->interruptNumber);
      return newSubscriber;
    }
    else {
      return getSyncRegisterAccessor<UserType>(registerPathName, numberOfWords, wordOffsetInRegister, flags);
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> NumericAddressedBackend::getSyncRegisterAccessor(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    boost::shared_ptr<NDRegisterAccessor<UserType>> accessor;
    // obtain register info
    boost::shared_ptr<RegisterInfo> info = getRegisterInfo(registerPathName);
    auto registerInfo = boost::static_pointer_cast<RegisterInfoMap::RegisterInfo>(info);

    // 1D or scalar register
    if(info->getNumberOfDimensions() <= 1) {
      if((registerInfo->dataType == RegisterInfoMap::RegisterInfo::Type::FIXED_POINT) ||
          (registerInfo->dataType == RegisterInfoMap::RegisterInfo::Type::VOID)) {
        if(flags.has(AccessMode::raw)) {
          accessor = boost::shared_ptr<NDRegisterAccessor<UserType>>(
              new NumericAddressedBackendRegisterAccessor<UserType, FixedPointConverter, true>(
                  shared_from_this(), registerPathName, numberOfWords, wordOffsetInRegister, flags));
        }
        else {
          accessor = boost::shared_ptr<NDRegisterAccessor<UserType>>(
              new NumericAddressedBackendRegisterAccessor<UserType, FixedPointConverter, false>(
                  shared_from_this(), registerPathName, numberOfWords, wordOffsetInRegister, flags));
        }
      }
      else if(registerInfo->dataType == RegisterInfoMap::RegisterInfo::Type::IEEE754) {
        if(flags.has(AccessMode::raw)) {
          accessor = boost::shared_ptr<NDRegisterAccessor<UserType>>(
              new NumericAddressedBackendRegisterAccessor<UserType, IEEE754_SingleConverter, true>(
                  shared_from_this(), registerPathName, numberOfWords, wordOffsetInRegister, flags));
        }
        else {
          accessor = boost::shared_ptr<NDRegisterAccessor<UserType>>(
              new NumericAddressedBackendRegisterAccessor<UserType, IEEE754_SingleConverter, false>(
                  shared_from_this(), registerPathName, numberOfWords, wordOffsetInRegister, flags));
        }
      }
      else {
        throw ChimeraTK::logic_error("NumericAddressedBackend:: trying to get "
                                     "accessor for unsupported data type");
      }
    }
    // 2D multiplexed register
    else {
      flags.checkForUnknownFlags({});
      accessor =
          boost::shared_ptr<NDRegisterAccessor<UserType>>(new NumericAddressedBackendMuxedRegisterAccessor<UserType>(
              registerPathName, numberOfWords, wordOffsetInRegister, shared_from_this()));
    }

    accessor->setExceptionBackend(shared_from_this());
    return accessor;
  }

  void NumericAddressedBackend::activateAsyncRead() noexcept {
    for(auto it : _interruptDispatchers) {
      it.second->activate();
    }
  }

  void NumericAddressedBackend::setException() {
    _hasActiveException = true;
    try {
      throw ChimeraTK::runtime_error("NumericAddressedBackend is in exception state.");
    }
    catch(...) {
      // FIXME: This is not thread-safe!
      for(auto it : _interruptDispatchers) {
        it.second->sendException(std::current_exception());
      }
    }
  }

  // empty default implementation
  void NumericAddressedBackend::startInterruptHandlingThread(
      [[maybe_unused]] unsigned int interruptControllerNumber, [[maybe_unused]] unsigned int interruptNumber) {}

  void NumericAddressedBackend::close() {
    for(auto it : _interruptDispatchers) {
      it.second->deactivate();
    }
    closeImpl();
  }
} // namespace ChimeraTK
