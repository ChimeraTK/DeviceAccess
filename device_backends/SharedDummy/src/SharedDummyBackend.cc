#include <algorithm>
#include <functional>
#include <sstream>
#include <regex>

#include <boost/filesystem.hpp>
#include <boost/lambda/lambda.hpp>

#include "BackendFactory.h"
#include "Exception.h"
#include "MapFileParser.h"
#include "SharedDummyBackend.h"
#include "parserUtilities.h"
#include "NumericAddressedInterruptDispatcher.h"

namespace ChimeraTK {

  SharedDummyBackend::SharedDummyBackend(std::string instanceId, std::string mapFileName)
  : DummyBackendBase(mapFileName), _mapFile(mapFileName), _barSizesInBytes(getBarSizesInBytesFromRegisterMapping()),
    sharedMemoryManager(*this, instanceId, mapFileName) {
    setupBarContents();
  }

  // Nothing to clean up, all objects clean up for themselves when
  // they go out of scope.
  SharedDummyBackend::~SharedDummyBackend() {}

  // Construct a segment for each bar and set required size
  void SharedDummyBackend::setupBarContents() {
    for(std::map<uint64_t, size_t>::const_iterator barSizeInBytesIter = _barSizesInBytes.begin();
        barSizeInBytesIter != _barSizesInBytes.end(); ++barSizeInBytesIter) {
      std::string barName = SHARED_MEMORY_BAR_PREFIX + std::to_string(barSizeInBytesIter->first);

      size_t barSizeInWords = (barSizeInBytesIter->second + sizeof(int32_t) - 1) / sizeof(int32_t);

      try {
        std::lock_guard<boost::interprocess::named_mutex> lock(sharedMemoryManager.interprocessMutex);
        _barContents[barSizeInBytesIter->first] = sharedMemoryManager.findOrConstructVector(barName, barSizeInWords);
      }
      catch(boost::interprocess::bad_alloc&) {
        // Clean up
        sharedMemoryManager.~SharedMemoryManager();

        std::string errMsg{"Could not allocate shared memory while constructing registers. "
                           "Please file a bug report at "
                           "https://github.com/ChimeraTK/DeviceAccess."};
        throw ChimeraTK::logic_error(errMsg);
      }
    } /* for(barSizesInBytesIter) */
  }

  void SharedDummyBackend::open() {
    _opened = true;
    _hasActiveException = false;
  }

  void SharedDummyBackend::closeImpl() { _opened = false; }

  void SharedDummyBackend::read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) {
    if(!_opened) {
      throw ChimeraTK::logic_error("Device is closed.");
    }
    if(_hasActiveException) {
      throw ChimeraTK::runtime_error("previous, unrecovered fault");
    }
    checkSizeIsMultipleOfWordSize(sizeInBytes);
    uint64_t wordBaseIndex = address / sizeof(int32_t);

    std::lock_guard<boost::interprocess::named_mutex> lock(sharedMemoryManager.interprocessMutex);

    for(uint64_t wordIndex = 0; wordIndex < sizeInBytes / sizeof(int32_t); ++wordIndex) {
      TRY_REGISTER_ACCESS(data[wordIndex] = _barContents[bar]->at(wordBaseIndex + wordIndex););
    }
  }

  void SharedDummyBackend::write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) {
    if(!_opened) {
      throw ChimeraTK::logic_error("Device is closed.");
    }
    if(_hasActiveException) {
      throw ChimeraTK::runtime_error("previous, unrecovered fault");
    }
    checkSizeIsMultipleOfWordSize(sizeInBytes);
    uint64_t wordBaseIndex = address / sizeof(int32_t);

    std::lock_guard<boost::interprocess::named_mutex> lock(sharedMemoryManager.interprocessMutex);

    for(uint64_t wordIndex = 0; wordIndex < sizeInBytes / sizeof(int32_t); ++wordIndex) {
      TRY_REGISTER_ACCESS(_barContents[bar]->at(wordBaseIndex + wordIndex) = data[wordIndex];);
    }
  }

  std::string SharedDummyBackend::readDeviceInfo() {
    std::stringstream info;
    info << "SharedDummyBackend"; // TODO add map file name again
    return info.str();
  }

  size_t SharedDummyBackend::getTotalRegisterSizeInBytes() const {
    size_t totalRegSize = 0;
    for(auto& pair : _barSizesInBytes) {
      totalRegSize += pair.second;
    }
    return totalRegSize;
  }

  void SharedDummyBackend::checkSizeIsMultipleOfWordSize(size_t sizeInBytes) {
    if(sizeInBytes % sizeof(int32_t)) {
      throw ChimeraTK::logic_error("Read/write size has to be a multiple of 4");
    }
  }

  boost::shared_ptr<DeviceBackend> SharedDummyBackend::createInstance(
      std::string address, std::map<std::string, std::string> parameters) {
    std::string mapFileName = parameters["map"];
    if(mapFileName == "") {
      throw ChimeraTK::logic_error("No map file name given.");
    }

    // when the factory is used to create the dummy device, mapfile path in the
    // dmap file is relative to the dmap file location. Converting the relative
    // mapFile path to an absolute path avoids issues when the dmap file is not
    // in the working directory of the application.
    return returnInstance<SharedDummyBackend>(address, address, convertPathRelativeToDmapToAbs(mapFileName));
  }

  std::string SharedDummyBackend::convertPathRelativeToDmapToAbs(const std::string& mapfileName) {
    std::string dmapDir = parserUtilities::extractDirectory(BackendFactory::getInstance().getDMapFilePath());
    std::string absPathToDmapDir = parserUtilities::convertToAbsolutePath(dmapDir);
    // the map file is relative to the dmap file location. Convert the relative
    // mapfilename to an absolute path
    boost::filesystem::path absPathToMapFile{parserUtilities::concatenatePaths(absPathToDmapDir, mapfileName)};
    // Possible ./, ../ elements are removed, as the path may be constructed
    // differently in different client applications
    return boost::filesystem::canonical(absPathToMapFile).string();
  }

  VersionNumber SharedDummyBackend::triggerInterrupt(int interruptControllerNumber, int interruptNumber) {
    // FIXME: This is the "one process" version taken from the regular DummyBackend. The correct implementation should
    // * Signal the interrupt to all processes which are accessing this shared memory (and to itself, depending on the implementation)
    // * Execute the dispatch (either based on the signal or directly here, depending on the implementation)
    try {
      return dispatchInterrupt(interruptControllerNumber, interruptNumber);
    }
    catch(std::out_of_range&) {
      throw ChimeraTK::logic_error("DummyBackend::triggerInterrupt(): Error: Unknown interrupt " +
          std::to_string(interruptControllerNumber) + ", " + std::to_string(interruptNumber));
    }
  }

} // Namespace ChimeraTK
