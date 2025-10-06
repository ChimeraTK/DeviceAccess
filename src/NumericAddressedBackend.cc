// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "NumericAddressedBackend.h"

#include "async/DomainImpl.h"
#include "async/DomainsContainer.h"
#include "Exception.h"
#include "MapFileParser.h"
#include "NumericAddress.h"
#include "NumericAddressedBackendASCIIAccessor.h"
#include "NumericAddressedBackendMuxedRegisterAccessor.h"
#include "NumericAddressedBackendRegisterAccessor.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace ChimeraTK {

  /********************************************************************************************************************/

  NumericAddressedBackend::NumericAddressedBackend(const std::string& mapFileName,
      std::unique_ptr<NumericAddressedRegisterCatalogue> registerMapPointer,
      const std::string& dataConsistencyKeyDescriptor)
  : _registerMapPointer(std::move(registerMapPointer)), _registerMap(*_registerMapPointer) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
    if(!mapFileName.empty()) {
      MapFileParser parser;
      std::tie(_registerMap, _metadataCatalogue) = parser.parse(mapFileName);
    }
    if(!dataConsistencyKeyDescriptor.empty()) {
      // parse as JSON
      try {
        auto jdescr = nlohmann::json::parse(dataConsistencyKeyDescriptor);
        for(const auto& el : jdescr.items()) {
          _registerMap.addDataConsistencyRealm(el.key(), el.value());
        }
      }
      catch(json::parse_error& e) {
        throw ChimeraTK::logic_error(std::format("Parsing DataConsistencyKeys parameter '{}' results in JSON error: {}",
            dataConsistencyKeyDescriptor, e.what()));
      }
    }
  }

  /********************************************************************************************************************/

  NumericAddressedRegisterInfo NumericAddressedBackend::getRegisterInfo(const RegisterPath& registerPathName) {
    if(!registerPathName.startsWith(numeric_address::BAR())) {
      return _registerMap.getBackendRegister(registerPathName);
    }
    /// FIXME move into catalogue implementation, then make return type a const reference!

    auto components = registerPathName.getComponents();
    if(components.size() != 3) {
      throw ChimeraTK::logic_error("Illegal numeric address: '" + (registerPathName) + "'");
    }
    auto bar = std::stoi(components[1]);
    size_t pos = components[2].find_first_of('*');
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

      return _asyncDomainsContainer.subscribe<NumericAddressedBackend, std::nullptr_t, UserType>(
          boost::static_pointer_cast<NumericAddressedBackend>(shared_from_this()),
          registerInfo.getQualifiedAsyncId().front(), _asyncIsActive, registerPathName, numberOfWords,
          wordOffsetInRegister, flags);
    }
    return getSyncRegisterAccessor<UserType>(registerPathName, numberOfWords, wordOffsetInRegister, flags);
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
              new NumericAddressedBackendRegisterAccessor<UserType, FixedPointConverter<DEPRECATED_FIXEDPOINT_DEFAULT>,
                  true>(shared_from_this(), registerPathName, numberOfWords, wordOffsetInRegister, flags));
        }
        else {
          accessor = boost::shared_ptr<NDRegisterAccessor<UserType>>(
              new NumericAddressedBackendRegisterAccessor<UserType, FixedPointConverter<DEPRECATED_FIXEDPOINT_DEFAULT>,
                  false>(shared_from_this(), registerPathName, numberOfWords, wordOffsetInRegister, flags));
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
        accessor =
            boost::shared_ptr<NDRegisterAccessor<UserType>>(new NumericAddressedBackendMuxedRegisterAccessor<UserType,
                FixedPointConverter<DEPRECATED_FIXEDPOINT_DEFAULT>>(
                registerPathName, numberOfWords, wordOffsetInRegister, shared_from_this()));
      }
    }

    accessor->setExceptionBackend(shared_from_this());
    return accessor;
  }

  /********************************************************************************************************************/

  void NumericAddressedBackend::activateAsyncRead() noexcept {
    _asyncIsActive = true;
    // Iterating all async domains must happen under the container lock. We prepare a lambda that is executed via
    // DomainsContainer::forEach().
    auto activateDomain = [this](size_t key, boost::shared_ptr<async::Domain>& domain) {
      auto domainImpl = boost::dynamic_pointer_cast<async::DomainImpl<std::nullptr_t>>(domain);
      assert(domainImpl);
      auto subscriptionDone = this->activateSubscription(key, domainImpl);
      // Wait until the backends reports that the subscription is complete (typically set from inside another thread)
      // before polling the initial values when activating the async domain. This is necessary to make sure we don't
      // miss an update that came in after polling the initial value.
      subscriptionDone.wait();
      domainImpl->activate(nullptr);
    };

    _asyncDomainsContainer.forEach(activateDomain);
  }

  /********************************************************************************************************************/

  // The default implementation just returns a ready future.
  std::future<void> NumericAddressedBackend::activateSubscription([[maybe_unused]] unsigned int interruptNumber,
      [[maybe_unused]] boost::shared_ptr<async::DomainImpl<std::nullptr_t>> asyncDomain) {
    std::promise<void> subscriptionDonePromise;
    subscriptionDonePromise.set_value();
    return subscriptionDonePromise.get_future();
  }

  /********************************************************************************************************************/

  void NumericAddressedBackend::close() {
    _asyncIsActive = false;

    _asyncDomainsContainer.forEach([](size_t, boost::shared_ptr<async::Domain>& domain) { domain->deactivate(); });

    closeImpl();
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

  void NumericAddressedBackend::setExceptionImpl() noexcept {
    _asyncIsActive = false;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
