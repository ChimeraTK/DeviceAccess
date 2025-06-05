// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SharedDummyBackend.h"
#include "Utilities.h"

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

  // Construct/deconstruct
  SharedDummyBackend::SharedMemoryManager::SharedMemoryManager(
      SharedDummyBackend& sharedDummyBackend_, std::size_t instanceIdHash, const std::string& mapFileName)
  : sharedDummyBackend(sharedDummyBackend_), name(Utilities::createShmName(instanceIdHash, mapFileName, getUserName())),
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

      pidSet = findOrConstructVector(SHARED_MEMORY_PID_SET_NAME, 0);

      // Clean up pidSet, if needed
      bool reInitRequired = checkPidSetConsistency();

      // If only "zombie" processes were found in PidSet,
      // reset data entries in shared memory.
      if(reInitRequired) {
        reInitMemory();
      }

      // Get memory item for version number
      requiredVersion = segment.find_or_construct<unsigned>(SHARED_MEMORY_REQUIRED_VERSION_NAME)(0);

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
    }
    catch(boost::interprocess::interprocess_exception&) {
      // interprocess_exception is only thrown if something seriously went wrong.
      // In this case we don't want anyone to catch it but terminate.
      std::terminate();
    }
    //  If size of pidSet is 0 (i.e, the instance belongs to the last accessing
    //  process), destroy shared memory and the interprocess mutex
    if(pidSetSize == 0) {
      boost::interprocess::shared_memory_object::remove(name.c_str());
      boost::interprocess::named_mutex::remove(name.c_str());
    }
  }

  // Member functions
  SharedMemoryVector* SharedDummyBackend::SharedMemoryManager::findOrConstructVector(
      const std::string& objName, const size_t size) {
    SharedMemoryVector* vector =
        segment.find_or_construct<SharedMemoryVector>(objName.c_str())(size, 0, sharedMemoryIntAllocator);

    return vector;
  }

  size_t SharedDummyBackend::SharedMemoryManager::getRequiredMemoryWithOverhead() {
    // Note: This uses _barSizeInBytes to determine number of vectors used,
    //       as it is initialized when this method gets called in the init list.
    return SHARED_MEMORY_OVERHEAD_PER_VECTOR * sharedDummyBackend._barSizesInBytes.size() +
        SHARED_MEMORY_CONST_OVERHEAD + sharedDummyBackend.getTotalRegisterSizeInBytes() + sizeof(ShmForSems);
  }

  std::pair<size_t, size_t> SharedDummyBackend::SharedMemoryManager::getInfoOnMemory() {
    return std::make_pair(segment.get_size(), segment.get_free_memory());
  }

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

  void SharedDummyBackend::SharedMemoryManager::reInitMemory() {
    std::vector<std::string> nameList = listNamedElements();

    for(auto& item : nameList) {
      if(item == SHARED_MEMORY_REQUIRED_VERSION_NAME) {
        segment.destroy<unsigned>(item.c_str());
      }
      // reset the BAR vectors in shm.
      // Note, InterruptDispatcherInterface uses unique_instance mechanism so it is not affected here
      else if(item != SHARED_MEMORY_PID_SET_NAME) {
        segment.destroy<SharedMemoryVector>(item.c_str());
      }
    }
    InterruptDispatcherInterface::cleanupShm(segment);
  }

  std::vector<std::string> SharedDummyBackend::SharedMemoryManager::listNamedElements() {
    std::vector<std::string> list(segment.get_num_named_objects());

    for(auto seg = segment.named_begin(); seg != segment.named_end(); ++seg) {
      list.emplace_back(seg->name());
    }
    return list;
  }

} /* namespace ChimeraTK */
