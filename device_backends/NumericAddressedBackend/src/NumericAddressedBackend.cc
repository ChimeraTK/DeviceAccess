// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "NumericAddressedBackend.h"

#include "Exception.h"
#include "MapFileParser.h"
#include "NumericAddress.h"
#include "NumericAddressedBackendASCIIAccessor.h"
#include "NumericAddressedBackendMuxedRegisterAccessor.h"
#include "NumericAddressedBackendRegisterAccessor.h"
#include "NumericAddressedInterruptDispatcher.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  NumericAddressedBackend::NumericAddressedBackend(
      std::string mapFileName, std::unique_ptr<NumericAddressedRegisterCatalogue> registerMapPointer)
  : _registerMapPointer(std::move(registerMapPointer)), _registerMap(*_registerMapPointer) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
    if(mapFileName != "") {
      MapFileParser parser;
      std::tie(_registerMap, _metadataCatalogue) = parser.parse(mapFileName);

      // create all the interrupt dispatchers that are described in the map file
      for(auto& interruptController : _registerMap.getListOfInterrupts()) {
        // interruptController is a pair<int, set<int>>, containing the controller number and a set of associated
        // interrupts
        for(auto interruptNumber : interruptController.second) {
          _interruptDispatchersNonConst[{interruptController.first, interruptNumber}] =
              boost::make_shared<NumericAddressedInterruptDispatcher>();
        }
      }
    }
  }

  /********************************************************************************************************************/

  NumericAddressedRegisterInfo NumericAddressedBackend::getRegisterInfo(const RegisterPath& registerPathName) {
    if(!registerPathName.startsWith(numeric_address::BAR)) {
      return _registerMap.getBackendRegister(registerPathName);
    }
    else {
      /// FIXME move into catalogue implementation, then make return type a const reference!

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
      return NumericAddressedRegisterInfo(registerPathName, nElements, address, nBytes, bar);
    }
  }

  /********************************************************************************************************************/

  /* Throw exception if called directly and not implemented by backend */
  void NumericAddressedBackend::read([[maybe_unused]] uint8_t bar, [[maybe_unused]] uint32_t address,
      [[maybe_unused]] int32_t* data, [[maybe_unused]] size_t sizeInBytes) {
    throw ChimeraTK::logic_error("NumericAddressedBackend: internal error: interface read() called w/ 32bit address");
  }

  /********************************************************************************************************************/

  void NumericAddressedBackend::write([[maybe_unused]] uint8_t bar, [[maybe_unused]] uint32_t address,
      [[maybe_unused]] int32_t const* data, [[maybe_unused]] size_t sizeInBytes) {
    throw ChimeraTK::logic_error("NumericAddressedBackend: internal error: interface write() called w/ 32bit address");
  }

  /********************************************************************************************************************/

  /* Call 32-bit address implementation by default, for backends that don't implement 64-bit */
  void NumericAddressedBackend::read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    read(static_cast<uint8_t>(bar), static_cast<uint32_t>(address), data, sizeInBytes);
#pragma GCC diagnostic pop
  }

  /********************************************************************************************************************/

  void NumericAddressedBackend::write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    write(static_cast<uint8_t>(bar), static_cast<uint32_t>(address), data, sizeInBytes);
#pragma GCC diagnostic pop
  }

  /********************************************************************************************************************/

  // Default range of valid BARs
  bool NumericAddressedBackend::barIndexValid(uint64_t bar) {
    return bar <= 5 || bar == 13;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> NumericAddressedBackend::getRegisterAccessor_impl(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    if(flags.has(AccessMode::wait_for_new_data)) {
      // get the interrupt information from the map file
      auto registerInfo = _registerMap.getBackendRegister(registerPathName);
      if(!registerInfo.getSupportedAccessModes().has(AccessMode::wait_for_new_data)) {
        throw ChimeraTK::logic_error(
            "Register " + registerPathName + " does not support AccessMode::wait_for_new_data.");
      }

      auto interruptDispatcher =
          _interruptDispatchers.at({registerInfo.interruptCtrlNumber, registerInfo.interruptNumber});
      assert(interruptDispatcher);
      auto newSubscriber = interruptDispatcher->subscribe<UserType>(
          boost::dynamic_pointer_cast<NumericAddressedBackend>(shared_from_this()), registerPathName, numberOfWords,
          wordOffsetInRegister, flags);
      // The new subscriber might already be activated. Hence the exception backend is already set by the interrupt
      // dispatcher.
      startInterruptHandlingThread(registerInfo.interruptCtrlNumber, registerInfo.interruptNumber);
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
    auto registerInfo = getRegisterInfo(registerPathName);

    // 1D or scalar register
    if(registerInfo.getNumberOfDimensions() <= 1) {
      if((registerInfo.channels.front().dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT) ||
          (registerInfo.channels.front().dataType == NumericAddressedRegisterInfo::Type::VOID)) {
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
      else if(registerInfo.channels.front().dataType == NumericAddressedRegisterInfo::Type::IEEE754) {
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
      else if(registerInfo.channels.front().dataType == NumericAddressedRegisterInfo::Type::ASCII) {
        if constexpr(!std::is_same<UserType, std::string>::value) {
          throw ChimeraTK::logic_error("NumericAddressedBackend: ASCII data must be read with std::string UserType.");
        }
        else {
          accessor = boost::shared_ptr<NDRegisterAccessor<UserType>>(new NumericAddressedBackendASCIIAccessor(
              shared_from_this(), registerPathName, numberOfWords, wordOffsetInRegister, flags));
        }
      }
      else {
        throw ChimeraTK::logic_error("NumericAddressedBackend: trying to get accessor for unsupported data type");
      }
    }
    // 2D multiplexed register
    else {
      flags.checkForUnknownFlags({});
      if(registerInfo.channels.front().dataType == NumericAddressedRegisterInfo::Type::IEEE754) {
        accessor = boost::shared_ptr<NDRegisterAccessor<UserType>>(
            new NumericAddressedBackendMuxedRegisterAccessor<UserType, IEEE754_SingleConverter>(
                registerPathName, numberOfWords, wordOffsetInRegister, shared_from_this()));
      }
      else {
        accessor = boost::shared_ptr<NDRegisterAccessor<UserType>>(
            new NumericAddressedBackendMuxedRegisterAccessor<UserType, FixedPointConverter>(
                registerPathName, numberOfWords, wordOffsetInRegister, shared_from_this()));
      }
    }

    accessor->setExceptionBackend(shared_from_this());
    return accessor;
  }

  /********************************************************************************************************************/

  void NumericAddressedBackend::activateAsyncRead() noexcept {
    for(auto& it : _interruptDispatchers) {
      it.second->activate();
    }
  }

  /********************************************************************************************************************/

  void NumericAddressedBackend::setException() {
    _hasActiveException = true;
    try {
      throw ChimeraTK::runtime_error("NumericAddressedBackend is in exception state.");
    }
    catch(...) {
      for(auto& it : _interruptDispatchers) {
        it.second->sendException(std::current_exception());
      }
    }
  }

  /********************************************************************************************************************/

  // empty default implementation
  void NumericAddressedBackend::startInterruptHandlingThread(
      [[maybe_unused]] unsigned int interruptControllerNumber, [[maybe_unused]] unsigned int interruptNumber) {}

  /********************************************************************************************************************/

  void NumericAddressedBackend::close() {
    for(const auto& it : _interruptDispatchers) {
      it.second->deactivate();
    }
    closeImpl();
  }

  /********************************************************************************************************************/

  VersionNumber NumericAddressedBackend::dispatchInterrupt(int interruptControllerNumber, int interruptNumber) {
    // This function just makes sure that at() is used to access the _interruptDispatchers map,
    // which guarantees that the map is not altered.
    return _interruptDispatchers.at({interruptControllerNumber, interruptNumber})->trigger();
  }

  /********************************************************************************************************************/

  RegisterCatalogue NumericAddressedBackend::getRegisterCatalogue() const {
    return RegisterCatalogue(_registerMap.clone());
  }

  /********************************************************************************************************************/

  MetadataCatalogue NumericAddressedBackend::getMetadataCatalogue() const {
    return _metadataCatalogue;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
