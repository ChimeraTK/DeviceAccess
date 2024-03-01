// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "BackendFactory.h"
#include "DummyRegisterAccessor.h"
#include "Exception.h"
#include "MapFileParser.h"
#include "parserUtilities.h"

#include <boost/lambda/lambda.hpp>

#include <algorithm>
#include <regex>
#include <sstream>

namespace ChimeraTK {

  DummyBackend::DummyBackend(const std::string& mapFileName) : DummyBackendBase(mapFileName), _mapFile(mapFileName) {
    resizeBarContents();
  }

  void DummyBackend::open() {
    std::lock_guard<std::mutex> lock(mutex);

    setOpenedAndClearException();
  }

  void DummyBackend::resizeBarContents() {
    std::lock_guard<std::mutex> lock(mutex);
    std::map<uint64_t, size_t> barSizesInBytes = getBarSizesInBytesFromRegisterMapping();

    for(auto& barSizesInByte : barSizesInBytes) {
      // the size of the vector is in words, not in bytes -> convert fist with rounding up
      auto nwords = (barSizesInByte.second + sizeof(int32_t) - 1) / sizeof(int32_t);
      _barContents[barSizesInByte.first].resize(nwords, 0);
    }
  }

  void DummyBackend::closeImpl() {
    std::lock_guard<std::mutex> lock(mutex);

    _opened = false;
  }

  void DummyBackend::writeRegisterWithoutCallback(uint64_t bar, uint64_t address, int32_t data) {
    std::lock_guard<std::mutex> lock(mutex);
    TRY_REGISTER_ACCESS(_barContents[bar].at(address / sizeof(int32_t)) = data;);
  }

  void DummyBackend::read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) {
    {
      std::lock_guard<std::mutex> lock(mutex);
      assert(_opened);
      checkActiveException();
      checkSizeIsMultipleOfWordSize(sizeInBytes);
      unsigned int wordBaseIndex = address / sizeof(int32_t);
      TRY_REGISTER_ACCESS(for(unsigned int wordIndex = 0; wordIndex < sizeInBytes / sizeof(int32_t);
                              ++wordIndex) { data[wordIndex] = _barContents[bar].at(wordBaseIndex + wordIndex); });
    }
  }

  void DummyBackend::write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) {
    {
      std::lock_guard<std::mutex> lock(mutex);
      assert(_opened);
      checkActiveException();
      checkSizeIsMultipleOfWordSize(sizeInBytes);
      uint64_t wordBaseIndex = address / sizeof(int32_t);
      TRY_REGISTER_ACCESS(for(unsigned int wordIndex = 0; wordIndex < sizeInBytes / sizeof(int32_t); ++wordIndex) {
        if(isReadOnly(bar, address + wordIndex * sizeof(int32_t))) {
          continue;
        }
        _barContents[bar].at(wordBaseIndex + wordIndex) = data[wordIndex];
      })
    }
    // we call the callback functions after releasing the mutex in order to
    // avoid the risk of deadlocks.
    runWriteCallbackFunctionsForAddressRange(AddressRange(bar, address, sizeInBytes));
  }

  std::string DummyBackend::readDeviceInfo() {
    std::stringstream info;
    info << "DummyBackend"; // TODO add map file name again
    return info.str();
  }

  void DummyBackend::setReadOnly(uint64_t bar, uint64_t address, size_t sizeInWords) {
    for(size_t i = 0; i < sizeInWords; ++i) {
      _readOnlyAddresses.insert({bar, address + i * sizeof(int32_t)});
    }
  }

  void DummyBackend::setReadOnly(AddressRange addressRange) {
    setReadOnly(addressRange.bar, addressRange.offset, addressRange.sizeInBytes / sizeof(int32_t));
  }

  bool DummyBackend::isReadOnly(uint64_t bar, uint64_t address) const {
    return (_readOnlyAddresses.find({bar, address}) != _readOnlyAddresses.end());
  }

  void DummyBackend::setWriteCallbackFunction(
      AddressRange addressRange, boost::function<void(void)> const& writeCallbackFunction) {
    _writeCallbackFunctions.insert(
        std::pair<AddressRange, boost::function<void(void)>>(addressRange, writeCallbackFunction));
  }

  void DummyBackend::runWriteCallbackFunctionsForAddressRange(AddressRange addressRange) {
    std::list<boost::function<void(void)>> callbackFunctionsForThisRange =
        findCallbackFunctionsForAddressRange(addressRange);
    for(auto& function : callbackFunctionsForThisRange) {
      function();
    }
  }

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

  bool DummyBackend::isWriteRangeOverlap(AddressRange firstRange, AddressRange secondRange) {
    if(firstRange.bar != secondRange.bar) {
      return false;
    }

    uint64_t startAddress = std::max(firstRange.offset, secondRange.offset);
    uint64_t endAddress =
        std::min(firstRange.offset + firstRange.sizeInBytes, secondRange.offset + secondRange.sizeInBytes);

    // if at least one register is writable there is an overlap of writable
    // registers
    for(uint64_t address = startAddress; address < endAddress; address += sizeof(int32_t)) {
      if(!isReadOnly(firstRange.bar, address)) {
        return true;
      }
    }

    // we looped all possible registers, none is writable
    return false;
  }

  boost::shared_ptr<DeviceBackend> DummyBackend::createInstance(
      // FIXME #11279 Implement API breaking changes from linter warnings
      // NOLINTNEXTLINE(performance-unnecessary-value-param)
      std::string address, std::map<std::string, std::string> parameters) {
    if(parameters["map"].empty()) {
      throw ChimeraTK::logic_error("No map file name given.");
    }

    // when the factory is used to create the dummy device, mapfile path in the
    // dmap file is relative to the dmap file location. Converting the relative
    // mapFile path to an absolute path avoids issues when the dmap file is not
    // in the working directory of the application.
    return returnInstance<DummyBackend>(address, convertPathRelativeToDmapToAbs(parameters["map"]));
  }

  std::string DummyBackend::convertPathRelativeToDmapToAbs(const std::string& mapfileName) {
    std::string dmapDir = parserUtilities::extractDirectory(BackendFactory::getInstance().getDMapFilePath());
    std::string absPathToDmapDir = parserUtilities::convertToAbsolutePath(dmapDir);
    // the map file is relative to the dmap file location. Convert the relative
    // mapfilename to an absolute path
    return parserUtilities::concatenatePaths(absPathToDmapDir, mapfileName);
  }

  DummyRegisterRawAccessor DummyBackend::getRawAccessor(const std::string& module, const std::string& register_name) {
    return {shared_from_this(), module, register_name};
  }

  VersionNumber DummyBackend::triggerInterrupt(uint32_t interruptNumber) {
    try {
      return dispatchInterrupt(interruptNumber);
    }
    catch(std::out_of_range&) {
      throw ChimeraTK::logic_error(
          "DummyBackend::triggerInterrupt(): Error: Unknown interrupt " + std::to_string(interruptNumber));
    }
  }

} // namespace ChimeraTK
