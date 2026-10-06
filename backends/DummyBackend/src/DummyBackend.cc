// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "BackendFactory.h"
#include "DummyRegisterAccessor.h"
#include "Exception.h"

#include <boost/lambda/lambda.hpp>

#include <algorithm>
#include <cstring>
#include <regex>
#include <sstream>

namespace ChimeraTK {

  DummyBackend::DummyBackend(const std::string& mapFileName, const std::string& dataConsistencyKeyDescriptor)
  : DummyBackendBase(mapFileName, dataConsistencyKeyDescriptor), _mapFile(mapFileName) {}

  /********************************************************************************************************************/

  void DummyBackend::open() {
    std::lock_guard<std::mutex> lock(mutex);

    setOpenedAndClearException();
  }

  /********************************************************************************************************************/

  std::byte* DummyBackend::getChunk(uint64_t bar, uint64_t chunkIndex) {
    // std::vector<std::byte> is value-initialised on first access because operator[] default-constructs the vector,
    // which yields a zero-initialised, CHUNK_SIZE big buffer.
    auto& chunk = _barContents[{bar, chunkIndex}];
    if(chunk.empty()) {
      chunk.resize(CHUNK_SIZE, std::byte{0});
    }
    return chunk.data();
  }

  /********************************************************************************************************************/

  void DummyBackend::closeImpl() {
    std::lock_guard<std::mutex> lock(mutex);

    _opened = false;
  }

  /********************************************************************************************************************/

  void DummyBackend::writeRegisterWithoutCallback(uint64_t bar, uint64_t address, int32_t data) {
    std::lock_guard<std::mutex> lock(mutex);
    transferChunked(
        bar, address, sizeof(int32_t), [this](uint64_t b, uint64_t chunkIndex) { return getChunk(b, chunkIndex); },
        [&data](size_t size, std::byte* chunk, size_t bytesDone) {
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          std::memcpy(chunk, reinterpret_cast<std::byte*>(&data) + bytesDone, size);
        });
  }

  /********************************************************************************************************************/

  void DummyBackend::read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) {
    std::lock_guard<std::mutex> lock(mutex);
    assert(_opened);
    checkActiveException();
    checkSizeIsMultipleOfWordSize(sizeInBytes);
    transferChunked(
        bar, address, sizeInBytes, [this](uint64_t b, uint64_t chunkIndex) { return getChunk(b, chunkIndex); },
        [data](size_t size, const std::byte* chunk, size_t bytesDone) {
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          std::memcpy(reinterpret_cast<std::byte*>(data) + bytesDone, chunk, size);
        });
  }

  /********************************************************************************************************************/

  void DummyBackend::write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) {
    {
      std::lock_guard<std::mutex> lock(mutex);
      assert(_opened);
      checkActiveException();
      checkSizeIsMultipleOfWordSize(sizeInBytes);
      transferChunked(
          bar, address, sizeInBytes, [this](uint64_t b, uint64_t chunkIndex) { return getChunk(b, chunkIndex); },
          [data](size_t size, std::byte* chunk, size_t bytesDone) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            std::memcpy(chunk, reinterpret_cast<const std::byte*>(data) + bytesDone, size);
          });
    }
    // we call the callback functions after releasing the mutex in order to
    // avoid the risk of deadlocks.
    runWriteCallbackFunctionsForAddressRange(AddressRange(bar, address, sizeInBytes));
  }

  /********************************************************************************************************************/

  std::string DummyBackend::readDeviceInfo() {
    std::stringstream info;
    info << "DummyBackend"; // TODO add map file name again
    return info.str();
  }

  /********************************************************************************************************************/

  void DummyBackend::setWriteCallbackFunction(
      AddressRange addressRange, boost::function<void(void)> const& writeCallbackFunction) {
    _writeCallbackFunctions.insert(
        std::pair<AddressRange, boost::function<void(void)>>(addressRange, writeCallbackFunction));
  }

  /********************************************************************************************************************/

  void DummyBackend::runWriteCallbackFunctionsForAddressRange(AddressRange addressRange) {
    std::list<boost::function<void(void)>> callbackFunctionsForThisRange =
        findCallbackFunctionsForAddressRange(addressRange);
    for(auto& function : callbackFunctionsForThisRange) {
      function();
    }
  }

  /********************************************************************************************************************/

  std::list<boost::function<void(void)>> DummyBackend::findCallbackFunctionsForAddressRange(AddressRange addressRange) {
    // as callback functions are not sortable, we want to loop the multimap only
    // once.
    // FIXME: If the same function is registered more than one, it may be executed
    // multiple times

    // we only want the start address of the range, so we set size to 0
    AddressRange firstAddressInBar(addressRange.bar, 0, 0);
    AddressRange endAddress(addressRange.bar, addressRange.offset + addressRange.sizeInBytes, 0);

    auto startIterator = _writeCallbackFunctions.lower_bound(firstAddressInBar);

    auto endIterator = _writeCallbackFunctions.lower_bound(endAddress);

    std::list<boost::function<void(void)>> returnList;
    for(auto callbackIter = startIterator; callbackIter != endIterator; ++callbackIter) {
      if(isWriteRangeOverlap(callbackIter->first, addressRange)) {
        returnList.push_back(callbackIter->second);
      }
    }

    return returnList;
  }

  /********************************************************************************************************************/

  bool DummyBackend::isWriteRangeOverlap(AddressRange firstRange, AddressRange secondRange) {
    if(firstRange.bar != secondRange.bar) {
      return false;
    }

    uint64_t startAddress = std::max(firstRange.offset, secondRange.offset);
    uint64_t endAddress =
        std::min(firstRange.offset + firstRange.sizeInBytes, secondRange.offset + secondRange.sizeInBytes);

    return startAddress < endAddress;
  }

  /********************************************************************************************************************/

  boost::shared_ptr<DeviceBackend> DummyBackend::createInstance(
      // FIXME #11279 Implement API breaking changes from linter warnings
      // NOLINTNEXTLINE(performance-unnecessary-value-param)
      std::string address, std::map<std::string, std::string> parameters) {
    if(parameters["map"].empty()) {
      throw ChimeraTK::logic_error("No map file name given.");
    }

    // the relative map file path is resolved (relative to the DMAP directory, then to the cwd) in the
    // NumericAddressedBackend base class
    return returnInstance<DummyBackend>(address, parameters["map"], parameters["DataConsistencyKeys"]);
  }

  /********************************************************************************************************************/

  VersionNumber DummyBackend::triggerInterrupt(uint32_t interruptNumber) {
    auto asyncDomain = boost::dynamic_pointer_cast<async::DomainImpl<std::nullptr_t>>(
        _asyncDomainsContainer.getDomain(interruptNumber));

    if(asyncDomain) {
      return asyncDomain->distribute(nullptr);
    }

    // If the asyncDomain is not there, the pointer in the _asyncDomainsContainer must be nullptr as well.
    // Otherwise the dynamic cast failed, which should never happen.
    assert(!_asyncDomainsContainer.getDomain(interruptNumber));
    return VersionNumber{nullptr};
  }

} // namespace ChimeraTK
