// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DummyBackendBase.h"
#include "Exception.h"
#include "NumericAddressedRegisterCatalogue.h"
#include "ProcessManagement.h"

#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/move/unique_ptr.hpp>
#include <boost/unordered_set.hpp>

#include <list>
#include <map>
#include <mutex>
#include <set>
#include <utility>
#include <vector>

// Define shared-memory compatible vector type and corresponding allocator
using ShmemAllocator =
    boost::interprocess::allocator<int32_t, boost::interprocess::managed_shared_memory::segment_manager>;
using SharedMemoryVector = boost::interprocess::vector<int32_t, ShmemAllocator>;
using PidSet = boost::interprocess::vector<int32_t, ShmemAllocator>;

namespace ChimeraTK {

  // max. allowed SharedDummyBackend instances using common shared mem segment (global count, over all processes)
  const int SHARED_MEMORY_N_MAX_MEMBER = 10;

  /** The shared dummy device opens a mapping file defining the registers and
   * implements them in shared memory instead of connecting to the real device.
   * Thus, it provides access to the registers from several applications. The
   * registers an application accesses can be stimulated or monitored by another
   * process, e.g. for development and testing.
   *
   *  Accessing applications are required to the same mapping file (matching
   * absolute path) and to be run by the same user.
   */
  class SharedDummyBackend : public DummyBackendBase {
   public:
    SharedDummyBackend(const std::string& instanceId, const std::string& mapFileName);
    ~SharedDummyBackend() override;

    void open() override;
    void closeImpl() override;

    using DummyBackendBase::read;  // use the 32 bit version from the base class
    using DummyBackendBase::write; // use the 32 bit version from the base class
    void read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) override;
    void write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) override;

    std::string readDeviceInfo() override;

    static boost::shared_ptr<DeviceBackend> createInstance(
        std::string address, std::map<std::string, std::string> parameters);

    VersionNumber triggerInterrupt(uint32_t interruptNumber) override;

   private:
    /** name of the map file */
    std::string _mapFile;

    // Bar contents with shared-memory compatible vector type. Plain pointers are
    // used here since this is what we get from the shared memory allocation.
    std::map<uint64_t, SharedMemoryVector*> _barContents;

    // Bar sizes
    std::map<uint64_t, size_t> _barSizesInBytes;

    // Naming of bars as shared memory elements
    const char* SHARED_MEMORY_BAR_PREFIX = "BAR_";

    class InterruptDispatcherInterface;

    // Helper class to manage the shared memory: automatically construct if
    // necessary, automatically destroy if last using process closes.
    class SharedMemoryManager {
      friend class SharedDummyBackend;

     public:
      SharedMemoryManager(SharedDummyBackend&, const std::string&, const std::string&);
      ~SharedMemoryManager();

      /**
       * Finds or constructs a vector object in the shared memory.
       */
      SharedMemoryVector* findOrConstructVector(const std::string& objName, size_t size);

      /**
       * Get information on the shared memory segment
       * @retval std::pair<size_t, size_t> first: Size of the memory segment,
       * second: free memory in segment
       */
      std::pair<size_t, size_t> getInfoOnMemory();

     private:
      // Constants to take overhead of managed shared memory into respect
      // (approx. linear function, meta data for memory and meta data per vector)
      // Uses overestimates for robustness.
      static const size_t SHARED_MEMORY_CONST_OVERHEAD = 1000;
      static const size_t SHARED_MEMORY_OVERHEAD_PER_VECTOR = 160;

      const char* SHARED_MEMORY_PID_SET_NAME = "PidSet";
      const char* SHARED_MEMORY_REQUIRED_VERSION_NAME = "RequiredVersion";

      SharedDummyBackend& sharedDummyBackend;

      // Hashes to assure match of shared memory accessing processes
      std::string userHash;
      std::string mapFileHash;
      std::string instanceIdHash;

      // the name of the segment
      std::string name;

      // the shared memory segment
      boost::interprocess::managed_shared_memory segment;

      // the allocator instance
      const ShmemAllocator sharedMemoryIntAllocator;

      // Pointers to the set of process IDs and the required version
      // specifier in shared memory
      PidSet* pidSet{nullptr};
      // Version number is not used for now, but included in shared memory
      // to facilitate compatibility checks later
      unsigned* requiredVersion{nullptr};

      size_t getRequiredMemoryWithOverhead();
      /**
       * Checks and if needed corrects the state of the pid set, i.e
       * if accessing processes have been terminated and could not clean up for
       * themselves, their entries are removed. This way, if at least the last
       * accessing process exits gracefully, the shared memory will be removed.
       * returns bool reInitRequired
       */
      bool checkPidSetConsistency();
      /// Resets all elements in shared memory except for the pidSet.
      void reInitMemory();
      std::vector<std::string> listNamedElements();

     protected:
      // interprocess mutex, has to be accessible by SharedDummyBackend class
      boost::interprocess::named_mutex interprocessMutex;

      boost::movelib::unique_ptr<InterruptDispatcherInterface> intDispatcherIf;
    }; /* class SharedMemoryManager */

    // Managed shared memory object
    SharedMemoryManager sharedMemoryManager;

    // Setup register bars in shared memory
    void setupBarContents();

    // Helper routines called in init list
    size_t getTotalRegisterSizeInBytes() const;

    static void checkSizeIsMultipleOfWordSize(size_t sizeInBytes);

    static std::string convertPathRelativeToDmapToAbs(std::string const& mapfileName);

    /****************** definitions for across-instance triggering ********/

    // We are using the process id as an id of the semaphore which is to be triggered for the interrupt dispatcher
    // thread. Since there is only one interrupt dispatcher thread per mapped shared memory region in a process, and
    // the semaphore is set inside the shared memory, this means we can identify all semaphores per shared memory
    // this way.
    // However, this implies the restriction that you must not create more than one backend instance per shared memory
    // region inside a process. E.g. if you wanted to write a test by tricking the backend factory into creating more
    // than one backend instance for the same process and shared memory, you will have a problem.
    using SemId = std::uint32_t;

    /// entry per semaphore in shared memory
    struct SemEntry {
      SemEntry() = default;
      boost::interprocess::interprocess_semaphore s{0};
      SemId semId{};
      bool used = false;
    };

    /// info about interrupt that can be placed in shm
    /// The _contollerID is conceptually wrong at this place. It is not used any more.
    /// We keep it and set it to 0 to have the shm compaitible with older versions.
    struct InterruptEntry {
      int _controllerId{0};
      int _intNumber{};
      std::uint32_t _counter = 0;
      bool used = false;
    };

    /// this limits the allowed number of different (controllerId, intNumber) pairs
    static const int maxInterruptEntries = 1000;

    /// Shm layout for semaphore management
    /// not thread safe
    struct ShmForSems {
      // In addition to the semaphores themselves, shm stores a vector of
      // interrupt numbers and their current counts.
      // Vector entries are not moved and marked as unused when no longer needed.
      // We need
      // - a find function which returns list of appropriate  semaphores to be triggered
      //   current concept does not use interrupt number for that
      // - add/remove functions to add/remove a semaphore
      // - functions to update interrupt counts

      using Sem = boost::interprocess::interprocess_semaphore;

      ShmForSems() = default;
      ShmForSems(const ShmForSems&) = delete;

      /// find unsed semaphore, mark it as used and return pointer to it
      Sem* addSem(SemId semId);
      bool removeSem(SemId semId);
      /// compare against PidSet and remove unused entries
      void cleanup(PidSet* pidSet);

      /// update shm entry to tell that interrupt is triggered
      /// implementation: increase interrupt count of given interrupt
      void addInterrupt(uint32_t interruptNumber);

      /// find list of semaphores to be triggered for given interrupt info
      /// if requested, update associated info
      /// i.e. store interrupt number so it can be found by triggered process
      std::list<Sem*> findSems(uint32_t interruptNumber = {}, bool update = false);

      /// for debugging purposes
      void print();

      SemEntry semEntries[SHARED_MEMORY_N_MAX_MEMBER];
      InterruptEntry interruptEntries[maxInterruptEntries];
    };

    struct InterruptDispatcherThread;

    class InterruptDispatcherInterface {
     public:
      /// adds semaphore & dispatcher thread on creation of interface
      /// shm will contain semaphore array and is protected by shmMutex
      /// <br>Since this object stores a reference to the backend it should be destroyed before the components
      /// of the backend required by the dispatcher thread
      InterruptDispatcherInterface(SharedDummyBackend& backend, boost::interprocess::managed_shared_memory& shm,
          boost::interprocess::named_mutex& shmMutex);

      /// stops dispatcher thread and removes semaphore
      ~InterruptDispatcherInterface();
      /// cleanup our objects in given shm. This is only needed when corrupt shm was detected which
      ///  needs re-initialization
      /// If pidSet given, remove unmatching entries. Otherwise, remove whole object in shm
      static void cleanupShm(boost::interprocess::managed_shared_memory& shm);
      static void cleanupShm(boost::interprocess::managed_shared_memory& shm, PidSet* pidSet);

      /// to be called from process which wishes to trigger some interrupt
      void triggerInterrupt(uint32_t intNumber);
      boost::interprocess::named_mutex& _shmMutex;
      SemId _semId;
      ShmForSems* _semBuf;
      boost::movelib::unique_ptr<InterruptDispatcherThread> _dispatcherThread;
      SharedDummyBackend& _backend;
    };

    struct InterruptDispatcherThread {
      /// starts dispatcher thread and then registers a semaphore of the semaphore array
      explicit InterruptDispatcherThread(InterruptDispatcherInterface* dispatcherInterf);
      InterruptDispatcherThread(const InterruptDispatcherThread&) = delete;
      /// stops and removes thread
      ~InterruptDispatcherThread();

      void run();
      void stop() noexcept;
      /// called for each interrupt event. Implements actual dispatching
      void handleInterrupt(uint32_t interruptNumber);

     private:
      // plain pointer, because of cyclic dependency
      InterruptDispatcherInterface* _dispatcherInterf;
      SemId _semId;
      ShmForSems* _semShm;
      ShmForSems::Sem* _sem = nullptr;
      boost::thread _thr;
      std::atomic_bool _started{false};
      std::atomic_bool _stop{false};
    };
  };
} // namespace ChimeraTK
