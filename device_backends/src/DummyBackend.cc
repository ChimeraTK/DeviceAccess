#include <algorithm>
#include <boost/lambda/lambda.hpp>
#include <sstream>

#include "BackendFactory.h"
#include "DummyBackend.h"
#include "Exception.h"
#include "MapFileParser.h"
#include "parserUtilities.h"
#include "DummyRegisterAccessor.h"

// macro to avoid code duplication
#define TRY_REGISTER_ACCESS(COMMAND)                                                                                   \
  try {                                                                                                                \
    COMMAND                                                                                                            \
  }                                                                                                                    \
  catch(std::out_of_range & outOfRangeException) {                                                                     \
    std::stringstream errorMessage;                                                                                    \
    errorMessage << "Invalid address offset " << address << " in bar " << static_cast<int>(bar) << "."                 \
                 << "Caught out_of_range exception: " << outOfRangeException.what();                                   \
    throw ChimeraTK::logic_error(errorMessage.str());                                                                  \
  }

namespace ChimeraTK {
  // Valid bar numbers are 0 to 5 , so they must be contained
  // in three bits.
  const unsigned int BAR_MASK = 0x7;
  // the bar number is stored in bits 60 to 62
  const unsigned int BAR_POSITION_IN_VIRTUAL_REGISTER = 60;

  DummyBackend::DummyBackend(std::string mapFileName) : NumericAddressedBackend(mapFileName), _mapFile(mapFileName) {
    _registerMap = MapFileParser().parse(_mapFile);
    _registerMapping = _registerMap;
    resizeBarContents();
  }

  // Nothing to clean up, all objects clean up for themselves when
  // they go out of scope.
  DummyBackend::~DummyBackend() {}

  void DummyBackend::open() {
    std::lock_guard<std::mutex> lock(mutex);
    if(_opened) {
      throw ChimeraTK::logic_error("Device is already open.");
    }
    _opened = true;
  }

  void DummyBackend::resizeBarContents() {
    std::lock_guard<std::mutex> lock(mutex);
    std::map<uint8_t, size_t> barSizesInBytes = getBarSizesInBytesFromRegisterMapping();

    for(std::map<uint8_t, size_t>::const_iterator barSizeInBytesIter = barSizesInBytes.begin();
        barSizeInBytesIter != barSizesInBytes.end(); ++barSizeInBytesIter) {
      // the size of the vector is in words, not in bytes -> convert fist
      _barContents[barSizeInBytesIter->first].resize(barSizeInBytesIter->second / sizeof(int32_t), 0);
    }
  }

  std::map<uint8_t, size_t> DummyBackend::getBarSizesInBytesFromRegisterMapping() const {
    std::map<uint8_t, size_t> barSizesInBytes;
    for(RegisterInfoMap::const_iterator mappingElementIter = _registerMapping->begin();
        mappingElementIter != _registerMapping->end(); ++mappingElementIter) {
      barSizesInBytes[mappingElementIter->bar] = std::max(barSizesInBytes[mappingElementIter->bar],
          static_cast<size_t>(mappingElementIter->address + mappingElementIter->nBytes));
    }
    return barSizesInBytes;
  }

  void DummyBackend::close() {
    std::lock_guard<std::mutex> lock(mutex);
    if(!_opened) {
      throw ChimeraTK::logic_error("Device is already closed.");
    }

    _readOnlyAddresses.clear();
    _writeCallbackFunctions.clear();
    _opened = false;
  }

  void DummyBackend::writeRegisterWithoutCallback(uint8_t bar, uint32_t address, int32_t data) {
    std::lock_guard<std::mutex> lock(mutex);
    TRY_REGISTER_ACCESS(_barContents[bar].at(address / sizeof(int32_t)) = data;);
  }

  void DummyBackend::read(uint8_t bar, uint32_t address, int32_t* data, size_t sizeInBytes) {
    std::lock_guard<std::mutex> lock(mutex);
    if(!_opened) {
      throw ChimeraTK::logic_error("Device is closed.");
    }
    checkSizeIsMultipleOfWordSize(sizeInBytes);
    unsigned int wordBaseIndex = address / sizeof(int32_t);
    TRY_REGISTER_ACCESS(for(unsigned int wordIndex = 0; wordIndex < sizeInBytes / sizeof(int32_t);
                            ++wordIndex) { data[wordIndex] = _barContents[bar].at(wordBaseIndex + wordIndex); });
  }

  void DummyBackend::write(uint8_t bar, uint32_t address, int32_t const* data, size_t sizeInBytes) {
    {
      std::lock_guard<std::mutex> lock(mutex);
      if(!_opened) {
        throw ChimeraTK::logic_error("Device is closed.");
      }
      checkSizeIsMultipleOfWordSize(sizeInBytes);
      unsigned int wordBaseIndex = address / sizeof(int32_t);
      TRY_REGISTER_ACCESS(for(unsigned int wordIndex = 0; wordIndex < sizeInBytes / sizeof(int32_t); ++wordIndex) {
        if(isReadOnly(bar, address + wordIndex * sizeof(int32_t))) {
          continue;
        }
        _barContents[bar].at(wordBaseIndex + wordIndex) = data[wordIndex];
      });
    }
    // we call the callback functions after releasing the mutex in order to
    // avoid the risk of deadlocks.
    runWriteCallbackFunctionsForAddressRange(AddressRange(bar, address, sizeInBytes));
  }

  std::string DummyBackend::readDeviceInfo() {
    std::stringstream info;
    info << "DummyBackend with mapping file " << _registerMapping->getMapFileName();
    return info.str();
  }

  uint64_t DummyBackend::calculateVirtualAddress(uint32_t registerOffsetInBar, uint8_t bar) {
    return (static_cast<uint64_t>(bar & BAR_MASK) << BAR_POSITION_IN_VIRTUAL_REGISTER) |
        (static_cast<uint64_t>(registerOffsetInBar));
  }

  void DummyBackend::checkSizeIsMultipleOfWordSize(size_t sizeInBytes) {
    if(sizeInBytes % sizeof(int32_t)) {
      throw ChimeraTK::logic_error("Read/write size has to be a multiple of 4");
    }
  }

  void DummyBackend::setReadOnly(uint8_t bar, uint32_t address, size_t sizeInWords) {
    for(size_t i = 0; i < sizeInWords; ++i) {
      uint64_t virtualAddress = calculateVirtualAddress(address + i * sizeof(int32_t), bar);
      _readOnlyAddresses.insert(virtualAddress);
    }
  }

  void DummyBackend::setReadOnly(AddressRange addressRange) {
    setReadOnly(addressRange.bar, addressRange.offset, addressRange.sizeInBytes / sizeof(int32_t));
  }

  bool DummyBackend::isReadOnly(uint8_t bar, uint32_t address) const {
    uint64_t virtualAddress = calculateVirtualAddress(address, bar);
    return (_readOnlyAddresses.find(virtualAddress) != _readOnlyAddresses.end());
  }

  void DummyBackend::setWriteCallbackFunction(
      AddressRange addressRange, boost::function<void(void)> const& writeCallbackFunction) {
    _writeCallbackFunctions.insert(
        std::pair<AddressRange, boost::function<void(void)>>(addressRange, writeCallbackFunction));
  }

  void DummyBackend::runWriteCallbackFunctionsForAddressRange(AddressRange addressRange) {
    std::list<boost::function<void(void)>> callbackFunctionsForThisRange =
        findCallbackFunctionsForAddressRange(addressRange);
    for(std::list<boost::function<void(void)>>::iterator functionIter = callbackFunctionsForThisRange.begin();
        functionIter != callbackFunctionsForThisRange.end(); ++functionIter) {
      (*functionIter)();
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

    std::multimap<AddressRange, boost::function<void(void)>>::iterator startIterator =
        _writeCallbackFunctions.lower_bound(firstAddressInBar);

    std::multimap<AddressRange, boost::function<void(void)>>::iterator endIterator =
        _writeCallbackFunctions.lower_bound(endAddress);

    std::list<boost::function<void(void)>> returnList;
    for(std::multimap<AddressRange, boost::function<void(void)>>::iterator callbackIter = startIterator;
        callbackIter != endIterator; ++callbackIter) {
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

    uint32_t startAddress = std::max(firstRange.offset, secondRange.offset);
    uint32_t endAddress =
        std::min(firstRange.offset + firstRange.sizeInBytes, secondRange.offset + secondRange.sizeInBytes);

    // if at least one register is writable there is an overlap of writable
    // registers
    for(uint32_t address = startAddress; address < endAddress; address += sizeof(int32_t)) {
      if(isReadOnly(firstRange.bar, address) == false) {
        return true;
      }
    }

    // we looped all possible registers, none is writable
    return false;
  }

  boost::shared_ptr<DeviceBackend> DummyBackend::createInstance(
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

  DummyRegisterRawAccessor DummyBackend::getRawAccessor(std::string module, std::string register_name) {
    return DummyRegisterRawAccessor(shared_from_this(), module, register_name);
  }

} // namespace ChimeraTK
