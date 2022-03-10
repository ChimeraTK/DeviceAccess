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
    this->sharedMemoryManager.intDispatcherIf->triggerInterrupt(interruptControllerNumber, interruptNumber);

    // Since VersionNumber consistency is defined only per process, we generate a new one here
    // and also in the triggered process
    return VersionNumber();
  }

  SharedDummyBackend::InterruptDispatcherInterface::InterruptDispatcherInterface(SharedDummyBackend& backend,
      boost::interprocess::managed_shared_memory& shm, boost::interprocess::named_mutex& shmMutex)
  : _shmMutex(shmMutex), _backend(backend) {
    // locking not needed, already defined as atomic
    _semBuf = shm.find_or_construct<ShmForSems>(ShmForSems::shmName)();
    _semId = getOwnPID();

    _dispatcherThread = boost::shared_ptr<InterruptDispatcherThread>(new InterruptDispatcherThread(this));
  }

  SharedDummyBackend::InterruptDispatcherInterface::~InterruptDispatcherInterface() {
    // stop thread and remove semaphore on destruction
    _dispatcherThread.reset(); // stops and deletes thread which uses semaphore
    std::lock_guard<boost::interprocess::named_mutex> lock(_shmMutex);
    _semBuf->removeSem(_semId);
  }

  void SharedDummyBackend::InterruptDispatcherInterface::triggerInterrupt(int controllerId, int intNumber) {
    std::list<boost::interprocess::interprocess_semaphore*> semList;
    {
      std::lock_guard<boost::interprocess::named_mutex> lock(_shmMutex);
      // find list of processes and their semaphores
      // update interrupt info
      semList = _semBuf->findSems(InterruptInfo(controllerId, intNumber), true);
    }
    // trigger the interrupts
    for(auto sem : semList) {
#ifdef _DEBUG
      std::cout << " InterruptDispatcherInterface::triggerInterrupt: post sem for interrupt: " << intNumber
                << std::endl;
      _semBuf->print();
#endif
      sem->post();
    }
  }

  SharedDummyBackend::InterruptDispatcherThread::InterruptDispatcherThread(
      InterruptDispatcherInterface* dispatcherInterf)
  : _dispatcherInterf(dispatcherInterf), _semId(dispatcherInterf->_semId), _semShm(dispatcherInterf->_semBuf) {
    _thr = new boost::thread(&InterruptDispatcherThread::run, this);
  }

  SharedDummyBackend::InterruptDispatcherThread::~InterruptDispatcherThread() {
    stop();
    _thr->join();
    delete _thr;
  }

  void SharedDummyBackend::InterruptDispatcherThread::run() {
    // copy interrupt counts at the beginning, and then
    // only look for different values. count up all values till they match
    // map (controller,intNumber) -> count
    // use map instead of vector because search is more efficient
    std::map<std::pair<int, int>, std::uint32_t> lastInterruptState;
    {
      std::lock_guard<boost::interprocess::named_mutex> lock(_dispatcherInterf->_shmMutex);
      for(auto& entry : _semShm->interruptEntries) {
        //lastInterruptState_.push_back(entry);
        lastInterruptState[std::make_pair(entry._controllerId, entry._intNumber)] = entry._counter;
      }
      _sem = _semShm->addSem(_semId);
    }

    while(!_stop) {
      _sem->wait();
      std::lock_guard<boost::interprocess::named_mutex> lock(_dispatcherInterf->_shmMutex);

      for(auto& entry : _semShm->interruptEntries) {
        if(!entry.used) continue;

        // find match with controllerId and intNumber
        auto key = std::make_pair(entry._controllerId, entry._intNumber);
        auto it = lastInterruptState.find(key);
        if(it != lastInterruptState.end()) {
          while(it->second != entry._counter) {
            // call trigger/dispatch
#ifdef _DEBUG
            std::cout << "existing interrupt event for x,y = " << entry._controllerId << ", " << entry._intNumber
                      << std::endl;
#endif
            handleInterrupt(entry._controllerId, entry._intNumber);
            it->second++;
          }
        }
        else {
          // new interrupt number
          // call trigger/dispatch count times
#ifdef _DEBUG
          std::cout << "count = " << entry._counter << " interrupt events for x,y = " << entry._controllerId << ", "
                    << entry._intNumber << std::endl;
#endif
          handleInterrupt(entry._controllerId, entry._intNumber);
          lastInterruptState[key] = entry._counter;
        }
      }
    }
  }

  void SharedDummyBackend::InterruptDispatcherThread::stop() {
    _stop = true;
    _sem->post();
  }

  void SharedDummyBackend::InterruptDispatcherThread::handleInterrupt(
      int interruptControllerNumber, int interruptNumber) {
    try {
      SharedDummyBackend& backend = _dispatcherInterf->_backend;
      VersionNumber vNumber = backend.dispatchInterrupt(interruptControllerNumber, interruptNumber);
    }
    catch(std::out_of_range&) {
      throw ChimeraTK::logic_error("InterruptDispatcherThread::triggerInterrupt(): Error: Unknown interrupt " +
          std::to_string(interruptControllerNumber) + ", " + std::to_string(interruptNumber));
    }
  }

  SharedDummyBackend::ShmForSems::Sem* SharedDummyBackend::ShmForSems::addSem(SemId semId) {
    // look up whether semaphore for id already exists and return error
    for(auto& entry : semEntries) {
      if(entry.used && entry.semId == semId)
        throw logic_error("error: semId already exists - check assumption about identifiers!");
    }

    for(auto& entry : semEntries) {
      if(!entry.used) {
        entry.semId = semId;
        entry.used = true;
        // It would be nice to also reset semaphores state, but if
        // interrupt dispatcher thread which last used it terminated property its not necessary
        // (since it calls post in destructor)
        return &entry.s;
      }
    }
    // increasing size not implemented
    throw runtime_error("error: semaphore array full - increase maxSems!");
  }

  void SharedDummyBackend::ShmForSems::removeSem(SemId semId) {
    for(auto& entry : semEntries) {
      if(entry.used && entry.semId == semId) {
        entry.used = 0;
        break;
      }
    }
  }

  void SharedDummyBackend::ShmForSems::addInterrupt(InterruptInfo ii) {
    bool found = false;
    for(auto& entry : interruptEntries) {
      if(entry.used && entry._controllerId == ii._controllerId && entry._intNumber == ii._intNumber) {
        entry._counter++;
        found = true;
        break;
      }
    }
    if(!found) {
      bool added = false;
      for(auto& entry : interruptEntries) {
        if(!entry.used) {
          entry.used = true;
          //entry.semId = semId;
          entry._controllerId = ii._controllerId;
          entry._intNumber = ii._intNumber;
          entry._counter = 1;
          added = true;
          break;
        }
      }
      if(!added) {
        throw runtime_error("no place left in interruptEntries!");
      }
    }
  }

  std::list<SharedDummyBackend::ShmForSems::Sem*> SharedDummyBackend::ShmForSems::findSems(
      InterruptInfo ii, bool update) {
    std::list<Sem*> ret;
    for(auto& entry : semEntries) {
      if(entry.used) {
        // we simply return all semaphores
        ret.push_back(&entry.s);
      }
    }
    if(update) addInterrupt(ii);
    return ret;
  }

  void SharedDummyBackend::ShmForSems::print() {
    std::cout << "shmem contents: " << std::endl;
    int i = 0;
    for(auto& entry : semEntries) {
      if(entry.used) std::cout << "sem : " << entry.semId << std::endl;
      i++;
    }
    i = 0;
    for(auto& entry : interruptEntries) {
      if(entry.used)
        std::cout << "interrupt : " << entry._controllerId << "," << entry._intNumber << " count = " << entry._counter
                  << std::endl;
      i++;
    }

    std::cout << std::endl;
  }

} // Namespace ChimeraTK
