// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ProcessManagement.h"
#include "SharedDummyBackend.h"
#include "Utilities.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <cstring>

namespace ChimeraTK {

  namespace {
    // Proxy class to add try_lock_for() to boost::interprocess::named_mutex::named_mutex, which is not present on older
    // BOOST versions like on Ubuntu 20.04. This can be removed and replaced with boost::interprocess::named_mutex once
    // we are fully on Ubuntu 24.04 or newer.
    class IpcNamedMutex {
     public:
      explicit IpcNamedMutex(boost::interprocess::named_mutex& mx) : _mx(mx) {}

      template<typename Duration>
      // NOLINTNEXTLINE(readability-identifier-naming)
      bool try_lock_for(const Duration& dur) {
        auto abs_time = boost::posix_time::microsec_clock::universal_time();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
        auto bmillis = boost::posix_time::milliseconds(millis);
        abs_time += bmillis;
        return _mx.timed_lock(abs_time);
      }

      template<typename Duration>
      // NOLINTNEXTLINE(readability-identifier-naming)
      bool try_lock_until(const Duration& abs_time) {
        return _mx.timed_lock(abs_time);
      }

      void lock() { _mx.lock(); }

      void unlock() { _mx.unlock(); }

      // NOLINTNEXTLINE(readability-identifier-naming)
      bool try_lock() { return _mx.try_lock(); }

     private:
      boost::interprocess::named_mutex& _mx;
    };
  } // namespace

  /********************************************************************************************************************/

  std::string SharedDummyBackend::getSharedMemoryName(
      std::size_t instanceIdHash, const std::string& mapFileName, const std::string& userName) {
    // Always use the absolute map file name, so the name does not depend on the spelling of the path.
    boost::filesystem::path absPathToMapFile = boost::filesystem::absolute(mapFileName);

    std::string mapFileHash{std::to_string(std::hash<std::string>{}(absPathToMapFile.string()))};
    std::string userHash{std::to_string(std::hash<std::string>{}(userName))};

    return "ChimeraTK_SharedDummy_" + std::to_string(instanceIdHash) + "_" + mapFileHash + "_" + userHash + "_v" +
        std::to_string(SHARED_MEMORY_LAYOUT_VERSION);
  }

  /********************************************************************************************************************/

  std::string SharedDummyBackend::SharedMemoryManager::getChunkSegmentName(uint64_t bar, uint64_t chunkIndex) const {
    return name + SHARED_MEMORY_CHUNK_SUFFIX + std::to_string(bar) + "_CHUNK_" + std::to_string(chunkIndex);
  }

  /********************************************************************************************************************/

  bool SharedDummyBackend::SharedMemoryManager::chunkSegmentExists(const std::string& segmentName) {
    boost::interprocess::shared_memory_object shm;
    try {
      shm = boost::interprocess::shared_memory_object(
          boost::interprocess::open_only, segmentName.c_str(), boost::interprocess::read_only);
    }
    catch(boost::interprocess::interprocess_exception&) {
      return false;
    }
    return true;
  }

  /********************************************************************************************************************/

  std::string SharedDummyBackend::SharedMemoryManager::walkToTail(const std::string& startName) {
    if(startName.empty() || !chunkSegmentExists(startName)) {
      return {};
    }

    std::string current = startName;
    while(true) {
      // Map the segment read-only to read its header.
      boost::interprocess::managed_shared_memory shm(boost::interprocess::open_only, current.c_str());
      auto* header = shm.find<SharedMemoryChunkHeader>(boost::interprocess::unique_instance).first;
      if(header == nullptr) {
        return current;
      }
      if(header->nextName[0] == '\0' || !chunkSegmentExists(header->nextName)) {
        return current;
      }
      current = header->nextName;
    }
  }

  /********************************************************************************************************************/

  void SharedDummyBackend::SharedMemoryManager::createChunk(uint64_t bar, uint64_t chunkIndex) {
    const std::string chunkName = getChunkSegmentName(bar, chunkIndex);
    std::unique_ptr<boost::interprocess::managed_shared_memory> chunkSegment;
    try {
      chunkSegment = std::make_unique<boost::interprocess::managed_shared_memory>(
          boost::interprocess::open_or_create, chunkName.c_str(), CHUNK_SEGMENT_SIZE);
      auto* header = chunkSegment->find_or_construct<SharedMemoryChunkHeader>(boost::interprocess::unique_instance)();
      header->bar = bar;
      header->chunkIndex = chunkIndex;
      header->nextName[0] = '\0';
      ShmemByteAllocator byteAllocator(chunkSegment->get_segment_manager());
      chunkSegment->find_or_construct<SharedMemoryByteVector>(SHARED_MEMORY_CHUNK_VECTOR_NAME)(
          CHUNK_SIZE, std::byte{0}, byteAllocator);
      _chunkSegments[{bar, chunkIndex}] = std::move(chunkSegment);
    }
    catch(boost::interprocess::interprocess_exception& e) {
      boost::interprocess::shared_memory_object::remove(chunkName.c_str());
      throw ChimeraTK::runtime_error(
          std::string("SharedDummyBackend: could not create shared memory chunk: ") + e.what());
    }
    catch(std::bad_alloc&) {
      boost::interprocess::shared_memory_object::remove(chunkName.c_str());
      throw ChimeraTK::runtime_error("SharedDummyBackend: could not create shared memory chunk: out of memory");
    }
  }

  /********************************************************************************************************************/

  void SharedDummyBackend::SharedMemoryManager::appendToChain(uint64_t bar, uint64_t chunkIndex) {
    const std::string chunkName = getChunkSegmentName(bar, chunkIndex);

    // Find the true tail: start at the tail if it names an existing segment, otherwise at the head.
    std::string tailName = walkToTail(_chunkList->tailName);
    if(tailName.empty()) {
      tailName = walkToTail(_chunkList->headName);
    }

    std::string previousLinkHolder;
    if(tailName.empty()) {
      // empty chain: the head will link to the new chunk
      previousLinkHolder.clear();
    }
    else {
      previousLinkHolder = tailName;
    }

    // Step 2: write the link before the segment exists, so it is always reachable from the head.
    if(previousLinkHolder.empty()) {
      std::strncpy(_chunkList->headName, chunkName.c_str(), SHARED_MEMORY_CHUNK_NAME_LENGTH - 1);
      _chunkList->headName[SHARED_MEMORY_CHUNK_NAME_LENGTH - 1] = '\0';
    }
    else {
      boost::interprocess::managed_shared_memory shm(boost::interprocess::open_only, previousLinkHolder.c_str());
      auto* header = shm.find<SharedMemoryChunkHeader>(boost::interprocess::unique_instance).first;
      assert(header != nullptr);
      std::strncpy(header->nextName, chunkName.c_str(), SHARED_MEMORY_CHUNK_NAME_LENGTH - 1);
      header->nextName[SHARED_MEMORY_CHUNK_NAME_LENGTH - 1] = '\0';
    }

    // Step 3: create the segment. Roll back the early link if this fails.
    try {
      createChunk(bar, chunkIndex);
    }
    catch(...) {
      if(previousLinkHolder.empty()) {
        _chunkList->headName[0] = '\0';
      }
      else {
        boost::interprocess::managed_shared_memory shm(boost::interprocess::open_only, previousLinkHolder.c_str());
        auto* header = shm.find<SharedMemoryChunkHeader>(boost::interprocess::unique_instance).first;
        if(header != nullptr) {
          header->nextName[0] = '\0';
        }
      }
      throw;
    }

    // Step 4: advance the tail only after the segment exists.
    std::strncpy(_chunkList->tailName, chunkName.c_str(), SHARED_MEMORY_CHUNK_NAME_LENGTH - 1);
    _chunkList->tailName[SHARED_MEMORY_CHUNK_NAME_LENGTH - 1] = '\0';
  }

  /********************************************************************************************************************/

  SharedMemoryByteVector* SharedDummyBackend::SharedMemoryManager::findOrConstructChunk(
      uint64_t bar, uint64_t chunkIndex) {
    const std::string chunkName = getChunkSegmentName(bar, chunkIndex);

    auto it = _chunkSegments.find({bar, chunkIndex});
    if(it != _chunkSegments.end()) {
      auto* chunk = it->second->find<SharedMemoryByteVector>(SHARED_MEMORY_CHUNK_VECTOR_NAME).first;
      assert(chunk != nullptr);
      return chunk;
    }

    // Fast path: the segment already exists (created by us or another process), just map it.
    if(chunkSegmentExists(chunkName)) {
      auto chunkSegment = std::make_unique<boost::interprocess::managed_shared_memory>(
          boost::interprocess::open_only, chunkName.c_str());
      auto* chunk = chunkSegment->find<SharedMemoryByteVector>(SHARED_MEMORY_CHUNK_VECTOR_NAME).first;
      assert(chunk != nullptr);
      _chunkSegments[{bar, chunkIndex}] = std::move(chunkSegment);
      return chunk;
    }

    appendToChain(bar, chunkIndex);
    auto* chunk =
        _chunkSegments.at({bar, chunkIndex})->find<SharedMemoryByteVector>(SHARED_MEMORY_CHUNK_VECTOR_NAME).first;
    assert(chunk != nullptr);
    return chunk;
  }

  /********************************************************************************************************************/

  // Construct/deconstruct
  SharedDummyBackend::SharedMemoryManager::SharedMemoryManager(
      SharedDummyBackend& sharedDummyBackend_, std::size_t instanceIdHash, const std::string& mapFileName)
  : sharedDummyBackend(sharedDummyBackend_),
    name(SharedDummyBackend::getSharedMemoryName(instanceIdHash, mapFileName, getUserName())),
    segment(boost::interprocess::open_or_create, name.c_str(), getRequiredMemoryWithOverhead()),
    sharedMemoryIntAllocator(segment.get_segment_manager()),
    interprocessMutex(boost::interprocess::open_or_create, name.c_str()) {
  retry:
    // scope for lock guard of the interprocess mutex
    {
      IpcNamedMutex proxy(interprocessMutex);
      std::unique_lock<IpcNamedMutex> lock(proxy, std::defer_lock);
      bool ok = lock.try_lock_for(std::chrono::milliseconds(2000));
      if(!ok) {
        std::cerr << "SharedDummyBackend: stale lock detected, removing mutex... " << std::endl;

        // named_mutex has no (move) assignment operator, so we need to work around here (placement delete and new)
        boost::interprocess::named_mutex::remove(name.c_str());
        interprocessMutex.~named_mutex();
        new(&interprocessMutex) boost::interprocess::named_mutex(boost::interprocess::open_or_create, name.c_str());

        goto retry;
      }

      pidSet = segment.find_or_construct<PidSet>(SHARED_MEMORY_PID_SET_NAME)(sharedMemoryIntAllocator);

      // head and tail of the chunk chain
      _chunkList = segment.find_or_construct<SharedMemoryChunkList>(SHARED_MEMORY_CHUNK_LIST_NAME)();

      // Clean up pidSet, if needed
      bool reInitRequired = checkPidSetConsistency();

      // If only "zombie" processes were found in PidSet,
      // reset data entries in shared memory.
      if(reInitRequired) {
        reInitMemory();
      }

      // Protect against too many accessing processes to prevent
      // overflow of pidSet in shared memory.
      if(pidSet->size() >= SHARED_MEMORY_N_MAX_MEMBER) {
        std::string errMsg{"Maximum number of accessing members reached."};
        throw ChimeraTK::runtime_error(errMsg);
      }
      InterruptDispatcherInterface::cleanupShm(segment, pidSet);

      pidSet->emplace_back(static_cast<int32_t>(getOwnPID()));
    } // releases the lock
    this->intDispatcherIf = boost::movelib::unique_ptr<InterruptDispatcherInterface>(
        new InterruptDispatcherInterface(sharedDummyBackend, segment, interprocessMutex));
  }

  /********************************************************************************************************************/

  SharedDummyBackend::SharedMemoryManager::~SharedMemoryManager() {
    // stop and delete dispatcher thread first since it uses shm and mutex
    intDispatcherIf.reset();
    size_t pidSetSize;
    try {
      // The scope of the try-block is the scope of the lock_guard, which can throw when locking.
      // All the lines in the try-block have to be executed under the lock, although not everything
      // might be throwing.

      // lock guard with the interprocess mutex
      std::lock_guard<boost::interprocess::named_mutex> lock(interprocessMutex);

      // Clean up
      checkPidSetConsistency();

      auto ownPid = static_cast<int32_t>(getOwnPID());
      for(auto it = pidSet->begin(); it != pidSet->end();) {
        if(*it == ownPid) {
          it = pidSet->erase(it);
        }
        else {
          ++it;
        }
      }
      pidSetSize = pidSet->size();

      // If we are the last process, remove the chunks before the home segment so a racing process sees an empty chain.
      if(pidSetSize == 0) {
        removeAllChunks();
      }
    }
    catch(boost::interprocess::interprocess_exception&) {
      // interprocess_exception is only thrown if something seriously went wrong.
      // In this case we don't want anyone to catch it but terminate.
      std::terminate();
    }
    //  If size of pidSet is 0 (i.e, the instance belongs to the last accessing
    //  process), destroy shared memory and the interprocess mutex
    if(pidSetSize == 0) {
      _chunkSegments.clear();
      boost::interprocess::shared_memory_object::remove(name.c_str());
      boost::interprocess::named_mutex::remove(name.c_str());
    }
  }

  /********************************************************************************************************************/

  void SharedDummyBackend::SharedMemoryManager::removeAllChunks() {
    // Step 1: save the head and clear the chain, so a racing process sees an empty chain.
    std::string current(_chunkList->headName);
    _chunkList->headName[0] = '\0';
    _chunkList->tailName[0] = '\0';

    // Step 2: remove the chunk segments by walking the saved chain.
    // A link may name a segment that does not exist: appendToChain() writes the link before creating the segment, so a
    // process killed in between leaves a dangling link. Such a name ends the walk and is skipped, not mapped, because
    // mapping it would throw a boost interprocess_exception and reach the std::terminate() handler above.
    while(!current.empty()) {
      if(!chunkSegmentExists(current)) {
        break;
      }
      boost::interprocess::managed_shared_memory shm(boost::interprocess::open_only, current.c_str());
      auto* header = shm.find<SharedMemoryChunkHeader>(boost::interprocess::unique_instance).first;
      std::string next;
      if(header != nullptr) {
        next = header->nextName;
      }
      boost::interprocess::shared_memory_object::remove(current.c_str());
      current = next;
    }

    _chunkSegments.clear();
  }

  /********************************************************************************************************************/

  size_t SharedDummyBackend::SharedMemoryManager::getRequiredMemoryWithOverhead() {
    // The home segment only holds the pid set, the chunk list and the interrupt storage. The chunks live in separate
    // segments, so the size is independent of the map file.
    return 4096 + sizeof(SharedMemoryChunkList) + sizeof(ShmForSems) + sizeof(PidSet) +
        SHARED_MEMORY_N_MAX_MEMBER * sizeof(int32_t);
  }

  /********************************************************************************************************************/

  std::pair<size_t, size_t> SharedDummyBackend::SharedMemoryManager::getInfoOnMemory() {
    return std::make_pair(segment.get_size(), segment.get_free_memory());
  }

  /********************************************************************************************************************/

  bool SharedDummyBackend::SharedMemoryManager::checkPidSetConsistency() {
    unsigned pidSetSizeBeforeCleanup = pidSet->size();

    for(auto it = pidSet->begin(); it != pidSet->end();) {
      if(!processExists(*it)) {
        // std::cout << "Nonexistent PID " << *it << " found. " <<std::endl;
        it = pidSet->erase(it);
      }
      else {
        it++;
      }
    }

    return pidSetSizeBeforeCleanup != 0 && pidSet->empty();
  }

  /********************************************************************************************************************/

  void SharedDummyBackend::SharedMemoryManager::reInitMemory() {
    removeAllChunks();
  }

  /********************************************************************************************************************/

} /* namespace ChimeraTK */
