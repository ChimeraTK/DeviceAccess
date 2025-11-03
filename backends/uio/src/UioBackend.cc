// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "UioBackend.h"

namespace ChimeraTK {

  UioBackend::UioBackend(
      const std::string& deviceName, const std::string& mapFileName, const std::string& dataConsistencyKeyDescriptor)
  : NumericAddressedBackend(
        mapFileName, std::make_unique<NumericAddressedRegisterCatalogue>(), dataConsistencyKeyDescriptor) {
    _uioAccess = std::make_shared<UioAccess>("/dev/" + deviceName);
  }

  UioBackend::~UioBackend() {
    UioBackend::closeImpl();
  }

  boost::shared_ptr<DeviceBackend> UioBackend::createInstance(
      // FIXME #11279 Implement API breaking changes from linter warnings
      // NOLINTNEXTLINE(performance-unnecessary-value-param)
      std::string address, std::map<std::string, std::string> parameters) {
    if(address.empty()) {
      throw ChimeraTK::logic_error("UIO: Device name not specified.");
    }

    if(parameters["map"].empty()) {
      throw ChimeraTK::logic_error("UIO: No map file name given.");
    }

    return boost::make_shared<UioBackend>(address, parameters["map"], parameters["DataConsistencyKeys"]);
  }

  void UioBackend::open() {
    if(_opened) {
      if(isFunctional()) {
        return;
      }
      close();
    }

    _uioAccess->open();
    setOpenedAndClearException();
  }

  void UioBackend::closeImpl() {
    if(_opened) {
      if(_interruptWaitingThread.joinable()) {
        _stopInterruptLoop = true;
        _interruptWaitingThread.join();
      }
      _asyncDomain.reset();
      _uioAccess->close();
    }

    _opened = false;
  }

  bool UioBackend::barIndexValid(uint64_t bar) {
    return (bar == 0);
  }

  void UioBackend::read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) {
    assert(_opened);
    checkActiveException();

    _uioAccess->read(bar, address, data, sizeInBytes);
  }

  void UioBackend::write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) {
    assert(_opened);
    checkActiveException();

    _uioAccess->write(bar, address, data, sizeInBytes);
  }

  std::future<void> UioBackend::activateSubscription(
      uint32_t interruptNumber, boost::shared_ptr<async::DomainImpl<std::nullptr_t>> asyncDomain) {
    std::promise<void> subscriptionDonePromise;

    if(interruptNumber != 0) {
      setException("UIO: Backend only uses interrupt number 0");
      subscriptionDonePromise.set_value();
      return subscriptionDonePromise.get_future();
    }
    if(_interruptWaitingThread.joinable()) {
      subscriptionDonePromise.set_value();
      return subscriptionDonePromise.get_future();
    }

    _stopInterruptLoop = false;
    _asyncDomain = asyncDomain;
    auto subscriptionDoneFuture = subscriptionDonePromise.get_future();
    _interruptWaitingThread = std::thread(&UioBackend::waitForInterruptLoop, this, std::move(subscriptionDonePromise));
    return subscriptionDoneFuture;
  }

  std::string UioBackend::readDeviceInfo() {
    std::string result = std::string("UIO backend: Device path = " + _uioAccess->getDeviceFilePath());
    if(!isOpen()) {
      result += " (device closed)";
    }

    return result;
  }

  void UioBackend::waitForInterruptLoop(std::promise<void> subscriptionDonePromise) {
    try { // also the scope for the promiseFulfiller

      // The NumericAddressedBackend is waiting for subscription done to be fulfilled, so it can continue with
      // polling the initial values. It is important to clear the old interrupts before this, so we don't clear any
      // interrupts that are triggered after the polling, but before this tread is ready to process them.
      //
      // We put the code into a finally so it is always executed when the promiseFulfiller goes out of scope, even if
      // there are exceptions in between.
      //
      auto promiseFulfiller = cppext::finally([&]() { subscriptionDonePromise.set_value(); });

      // This also enables the interrupts if they are not active.
      _uioAccess->clearInterrupts();

      // Clearing active interrupts actually is only effective after a poll (inside waitForInterrupt)
      if(_uioAccess->waitForInterrupt(0) > 0) {
        _uioAccess->clearInterrupts();
      }
    } // end of the promiseFulfiller scope
    catch(ChimeraTK::runtime_error& e) {
      setException(e.what());
      return;
    }

    while(!_stopInterruptLoop) {
      try {
        auto numberOfInterrupts = _uioAccess->waitForInterrupt(100);

        if(numberOfInterrupts > 0) {
          _uioAccess->clearInterrupts();

          if(!isFunctional()) {
            break;
          }

#ifdef _DEBUG_UIO
          if(numberOfInterrupts > 1) {
            std::cout << "UioBackend: Lost " << (numberOfInterrupts - 1) << " interrupts. " << std::endl;
          }
          std::cout << "dispatching interrupt " << std::endl;
#endif

          _asyncDomain->distribute(nullptr);
        }
      }
      catch(ChimeraTK::runtime_error& ex) {
        setException(ex.what());
        break;
      }
    }
  }

} // namespace ChimeraTK
