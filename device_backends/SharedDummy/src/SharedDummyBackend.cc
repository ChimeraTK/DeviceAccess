// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SharedDummyBackend.h"

#include "BackendFactory.h"
#include "Exception.h"
#include "MapFileParser.h"
#include "parserUtilities.h"

#include <boost/filesystem.hpp>
#include <boost/lambda/lambda.hpp>

#include <algorithm>
#include <cstring>
#include <functional>
#include <regex>
#include <sstream>

namespace ChimeraTK {

  SharedDummyBackend::SharedDummyBackend(const std::string& instanceId, const std::string& mapFileName)
  : DummyBackendBase(mapFileName), _mapFile(mapFileName), _barSizesInBytes(getBarSizesInBytesFromRegisterMapping()),
    sharedMemoryManager(*this, instanceId, mapFileName) {
    setupBarContents();
  }

  SharedDummyBackend::~SharedDummyBackend() {
    // Destroy the InterruptDispatcherInterface first because it keeps a reference to this backend.
    sharedMemoryManager.intDispatcherIf.reset();
    // all other objects clean up for themselves when they go out of scope.
  }

  // Construct a segment for each bar and set required size
  void SharedDummyBackend::setupBarContents() {
    for(auto& _barSizesInByte : _barSizesInBytes) {
      std::string barName = SHARED_MEMORY_BAR_PREFIX + std::to_string(_barSizesInByte.first);

      size_t barSizeInWords = (_barSizesInByte.second + sizeof(int32_t) - 1) / sizeof(int32_t);

      try {
        std::lock_guard<boost::interprocess::named_mutex> lock(sharedMemoryManager.interprocessMutex);
        _barContents[_barSizesInByte.first] = sharedMemoryManager.findOrConstructVector(barName, barSizeInWords);
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
    setOpenedAndClearException();
  }

  void SharedDummyBackend::closeImpl() {
    _opened = false;
  }

  void SharedDummyBackend::read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) {
    if(!_opened) {
      throw ChimeraTK::logic_error("Device is closed.");
    }
    checkActiveException();
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
    checkActiveException();
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
    for(const auto& pair : _barSizesInBytes) {
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
    if(mapFileName.empty()) {
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

  VersionNumber SharedDummyBackend::triggerInterrupt(uint32_t interruptNumber) {
    this->sharedMemoryManager.intDispatcherIf->triggerInterrupt(interruptNumber);

    // Since VersionNumber consistency is defined only per process, we generate a new one here
    // and also in the triggered process
    return {};
  }

  SharedDummyBackend::InterruptDispatcherInterface::InterruptDispatcherInterface(SharedDummyBackend& backend,
      boost::interprocess::managed_shared_memory& shm, boost::interprocess::named_mutex& shmMutex)
  : _shmMutex(shmMutex), _backend(backend) {
    // locking not needed, already defined as atomic
    _semBuf = shm.find_or_construct<ShmForSems>(boost::interprocess::unique_instance)();
    _semId = getOwnPID();

    _dispatcherThread = boost::movelib::unique_ptr<InterruptDispatcherThread>(new InterruptDispatcherThread(this));
  }

  SharedDummyBackend::InterruptDispatcherInterface::~InterruptDispatcherInterface() {
    // stop thread and remove semaphore on destruction
    _dispatcherThread.reset(); // stops and deletes thread which uses semaphore
    try {
      // The scope of the try-block is the scope of the lock_guard, which can throw when locking.
      // All the lines in the try-block have to be executed under the lock, although not everything
      // might be throwing.

      std::lock_guard<boost::interprocess::named_mutex> lock(_shmMutex);
      _semBuf->removeSem(_semId);
    }
    catch(boost::interprocess::interprocess_exception&) {
      // interprocess_exception is only thrown if something seriously went wrong.
      // In this case we don't want anyone to catch it but terminate.
      std::terminate();
    }
  }

  void SharedDummyBackend::InterruptDispatcherInterface::cleanupShm(boost::interprocess::managed_shared_memory& shm) {
    shm.destroy<SharedMemoryVector>(boost::interprocess::unique_instance);
  }
  void SharedDummyBackend::InterruptDispatcherInterface::cleanupShm(
      boost::interprocess::managed_shared_memory& shm, PidSet* pidSet) {
    ShmForSems* semBuf = shm.find_or_construct<ShmForSems>(boost::interprocess::unique_instance)();
    semBuf->cleanup(pidSet);
  }

  void SharedDummyBackend::InterruptDispatcherInterface::triggerInterrupt(uint32_t intNumber) {
    std::list<boost::interprocess::interprocess_semaphore*> semList;
    {
      std::lock_guard<boost::interprocess::named_mutex> lock(_shmMutex);
      // find list of processes and their semaphores
      // update interrupt info.
      semList = _semBuf->findSems(intNumber, true);
    }
    // trigger the interrupts
    for(auto* sem : semList) {
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
    _thr = boost::thread(&InterruptDispatcherThread::run, this);
  }

  SharedDummyBackend::InterruptDispatcherThread::~InterruptDispatcherThread() {
    stop();
    try {
      _thr.join();
    }

    catch(boost::system::system_error&) {
      std::terminate();
    }
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
        assert(entry._controllerId == 0);
        if(!entry.used) continue;
        lastInterruptState[std::make_pair(entry._controllerId, entry._intNumber)] = entry._counter;
      }
      // we register a semaphore only after being ready
      _sem = _semShm->addSem(_semId);
      _started = true;
    }

    // local copy of shm contents, used to reduce lock time
    InterruptEntry interruptEntries[maxInterruptEntries];

    while(!_stop) {
      _sem->wait();
      {
        std::lock_guard<boost::interprocess::named_mutex> lock(_dispatcherInterf->_shmMutex);
        std::memcpy(interruptEntries, _semShm->interruptEntries, sizeof(interruptEntries));
      }
      for(auto& entry : interruptEntries) {
        assert(entry._controllerId == 0);
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
            handleInterrupt(entry._intNumber);
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
          handleInterrupt(entry._intNumber);
          lastInterruptState[key] = entry._counter;
        }
      }
    }
  }

  void SharedDummyBackend::InterruptDispatcherThread::stop() noexcept {
    _stop = true;
    // we must wait until the semaphore is registered
    try {
      while(!_started) {
        boost::this_thread::sleep_for(boost::chrono::milliseconds{10});
      }
    }
    catch(const boost::thread_interrupted&) {
      // Simply suppress the thread_interrupted here. This function is only called
      // within a destructor, which anyway would have terminated the program when it sees the exception.
      // There are two possible scenarios what can happen now.
      // 1. _started is set and the destruction can continue normally.
      // 2. _started is not set yet an we don't know if the semaphore is in the correct state.
      //    Again there are two possibilities:
      //    2a_ The semaphore was set correctly and the destructor continues normally.
      //    2b_ sem->post() throws and terminate() is called (which otherwise would have been called from the escaping
      //    thread_interrupted
    }
    catch(const boost::system::system_error&) {
      // if something went really wrong we terminate here
      std::terminate();
    }

    try {
      _sem->post();
    }
    catch(const boost::interprocess::interprocess_exception&) {
      std::terminate();
    }
  }

  void SharedDummyBackend::InterruptDispatcherThread::handleInterrupt(uint32_t interruptNumber) {
    try {
      SharedDummyBackend& backend = _dispatcherInterf->_backend;
      backend.dispatchInterrupt(interruptNumber);
    }
    catch(std::out_of_range&) {
      throw ChimeraTK::logic_error(
          "InterruptDispatcherThread::triggerInterrupt(): Error: Unknown interrupt " + std::to_string(interruptNumber));
    }
  }

  SharedDummyBackend::ShmForSems::Sem* SharedDummyBackend::ShmForSems::addSem(SemId semId) {
    // look up whether semaphore for id already exists and return error
    for(auto& entry : semEntries) {
      if(entry.used && entry.semId == semId) {
        throw logic_error("error: semId already exists - check assumption about identifiers!");
      }
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

  bool SharedDummyBackend::ShmForSems::removeSem(SemId semId) {
    bool found = false;
    for(auto& entry : semEntries) {
      if(entry.used && entry.semId == semId) {
        entry.used = false;
        found = true;
        break;
      }
    }
    return found;
  }
  void SharedDummyBackend::ShmForSems::cleanup(PidSet* pidSet) {
    for(auto& entry : semEntries) {
      if(entry.used) {
        if(std::find(std::begin(*pidSet), std::end(*pidSet), (int32_t)entry.semId) == std::end(*pidSet)) {
          entry.used = false;
        }
      }
    }
  }

  void SharedDummyBackend::ShmForSems::addInterrupt(uint32_t interruptNumber) {
    bool found = false;
    for(auto& entry : interruptEntries) {
      if(entry.used && entry._controllerId == 0 && static_cast<uint32_t>(entry._intNumber) == interruptNumber) {
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
          entry._controllerId = 0;
          entry._intNumber = static_cast<int>(interruptNumber);
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
      uint32_t interruptNumber, bool update) {
    std::list<Sem*> ret;
    for(auto& entry : semEntries) {
      if(entry.used) {
        // we simply return all semaphores
        ret.push_back(&entry.s);
      }
    }
    if(update) addInterrupt(interruptNumber);
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
      if(entry.used) {
        std::cout << "interrupt : " << entry._controllerId << "," << entry._intNumber << " count = " << entry._counter
                  << std::endl;
      }
      i++;
    }

    std::cout << std::endl;
  }

} // Namespace ChimeraTK
