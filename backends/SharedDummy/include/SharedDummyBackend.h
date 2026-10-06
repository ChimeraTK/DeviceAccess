// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DummyBackendBase.h"

#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/move/unique_ptr.hpp>
#include <boost/unordered_set.hpp>

#include <cstddef>
#include <list>
#include <map>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// Define shared-memory compatible vector type and corresponding allocator
using ShmemAllocator =
    boost::interprocess::allocator<int32_t, boost::interprocess::managed_shared_memory::segment_manager>;
using PidSet = boost::interprocess::vector<int32_t, ShmemAllocator>;

// Separate byte allocator and vector type for the lazily materialised storage chunks.
using ShmemByteAllocator =
    boost::interprocess::allocator<std::byte, boost::interprocess::managed_shared_memory::segment_manager>;
using SharedMemoryByteVector = boost::interprocess::vector<std::byte, ShmemByteAllocator>;

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
    SharedDummyBackend(
        size_t instanceIdHash, const std::string& mapFileName, const std::string& dataConsistencyKeyDescriptor = "");
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

    /// Version of the shared-memory layout. It is appended to the segment base name, so processes running different
    /// library versions derive different names and never attach to each other's objects, in either direction.
    static constexpr unsigned SHARED_MEMORY_LAYOUT_VERSION = 2;

    /// Compute the (versioned) shared-memory base name for the given instance. All segments (the home segment, the
    /// interprocess mutex and every chunk segment) derive from this name.
    static std::string getSharedMemoryName(
        std::size_t instanceIdHash, const std::string& mapFileName, const std::string& userName);

   private:
    /** name of the map file */
    std::string _mapFile;

    /// Bar contents with shared-memory compatible byte vector type, keyed by (bar, chunkIndex). Plain pointers are
    /// used since this is what we get from the shared memory allocation. Each value is a CHUNK_SIZE big,
    /// zero-initialised chunk.
    std::map<std::pair<uint64_t, uint64_t>, SharedMemoryByteVector*> _barContents;

    /// Number of characters reserved for a segment name in the chunk chain header.
    static constexpr size_t SHARED_MEMORY_CHUNK_NAME_LENGTH = 512;

    /// Header of a chunk segment. It stores the chunk's (bar, chunkIndex) key and the name of the next segment in the
    /// chain, empty for the last one.
    struct SharedMemoryChunkHeader {
      uint64_t bar{};
      uint64_t chunkIndex{};
      // NOLINTNEXTLINE(modernize-avoid-c-arrays)
      char nextName[SHARED_MEMORY_CHUNK_NAME_LENGTH]{};
    };

    /// Anchors of the singly linked list of chunk segments, stored in the home segment. They name, rather than map,
    /// the first and last segment, because mapped addresses differ between processes.
    struct SharedMemoryChunkList {
      // NOLINTNEXTLINE(modernize-avoid-c-arrays)
      char headName[SHARED_MEMORY_CHUNK_NAME_LENGTH]{};
      // NOLINTNEXTLINE(modernize-avoid-c-arrays)
      char tailName[SHARED_MEMORY_CHUNK_NAME_LENGTH]{};
    };

    /// Suffix of the shared-memory object holding a chunk segment.
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    static constexpr char SHARED_MEMORY_CHUNK_SUFFIX[] = "_BAR_";

    /// Name of the shared-memory object holding the pid set in the home segment.
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    static constexpr char SHARED_MEMORY_PID_SET_NAME[] = "PidSet";

    /// Name of the shared-memory object holding the chunk chain anchors in the home segment.
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    static constexpr char SHARED_MEMORY_CHUNK_LIST_NAME[] = "ChunkList";

    /// Name of the shared-memory object holding a chunk's bytes inside its own segment.
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    static constexpr char SHARED_MEMORY_CHUNK_VECTOR_NAME[] = "chunk";

    /// Size of a chunk segment: one CHUNK_SIZE big vector plus the fixed header and the allocator overhead.
    static constexpr size_t CHUNK_SEGMENT_SIZE = CHUNK_SIZE + 65536;

    class InterruptDispatcherInterface;

    // Helper class to manage the shared memory: automatically construct if
    // necessary, automatically destroy if last using process closes.
    class SharedMemoryManager {
      friend class SharedDummyBackend;

     public:
      SharedMemoryManager(SharedDummyBackend&, std::size_t instanceIdHash, const std::string&);
      ~SharedMemoryManager();

      /// Return the chunk for the given (bar, chunkIndex), creating it zero-initialised on first access. The
      /// interprocess mutex must be held by the caller.
      SharedMemoryByteVector* findOrConstructChunk(uint64_t bar, uint64_t chunkIndex);

      /**
       * Get information on the shared memory segment
       * @retval std::pair<size_t, size_t> first: Size of the memory segment,
       * second: free memory in segment
       */
      std::pair<size_t, size_t> getInfoOnMemory();

     private:
      SharedDummyBackend& sharedDummyBackend;

      // the name of the home segment (and the base of all derived names)
      std::string name;

      // the shared memory home segment
      boost::interprocess::managed_shared_memory segment;

      // the allocator instance for the pid set
      const ShmemAllocator sharedMemoryIntAllocator;

      // Pointers to the set of process IDs in shared memory
      PidSet* pidSet{nullptr};

      // head and tail of the chunk chain
      SharedMemoryChunkList* _chunkList{nullptr};

      /// The mapped chunk segments, keyed by (bar, chunkIndex). Keeps the shared memory objects alive for as long as
      /// the process uses them, because the chunk vectors are only valid while their segment is mapped. Cached
      /// SharedMemoryByteVector pointers stay valid because chunks are never relocated.
      std::map<std::pair<uint64_t, uint64_t>, std::unique_ptr<boost::interprocess::managed_shared_memory>>
          _chunkSegments;

      /// size of the home segment
      size_t getRequiredMemoryWithOverhead();

      /// the full name of a chunk segment
      [[nodiscard]] std::string getChunkSegmentName(uint64_t bar, uint64_t chunkIndex) const;

      /// whether a chunk segment with the given name currently exists
      static bool chunkSegmentExists(const std::string& segmentName);

      /// Follow the chain from the given start name to its last existing segment. Returns an empty string when the
      /// chain starting there is empty.
      static std::string walkToTail(const std::string& startName);

      /// Create the chunk segment for the given key and map it. The interprocess mutex must be held by the caller.
      /// On failure the created (or partially created) segment is removed and the error is propagated as
      /// ChimeraTK::runtime_error.
      void createChunk(uint64_t bar, uint64_t chunkIndex);

      /// Append a chunk to the chain in a crash-safe order. The interprocess mutex must be held by the caller.
      void appendToChain(uint64_t bar, uint64_t chunkIndex);

      /**
       * Checks and if needed corrects the state of the pid set, i.e
       * if accessing processes have been terminated and could not clean up for
       * themselves, their entries are removed. This way, if at least the last
       * accessing process exits gracefully, the shared memory will be removed.
       * returns bool reInitRequired
       */
      bool checkPidSetConsistency();
      /// Walks the chunk chain and resets/removes the chunk segments, except for the pidSet.
      void reInitMemory();
      /// Removes every chunk segment of the chain and clears the head and tail.
      void removeAllChunks();

     protected:
      // interprocess mutex, has to be accessible by SharedDummyBackend class
      boost::interprocess::named_mutex interprocessMutex;

      boost::movelib::unique_ptr<InterruptDispatcherInterface> intDispatcherIf;
    }; /* class SharedMemoryManager */

    // Managed shared memory object
    std::unique_ptr<SharedMemoryManager> sharedMemoryManager;

    // Reset the per-process chunk map. The chunks themselves live in shared memory and are managed by the chain.
    void setupBarContents();

    /// Return a pointer to the requested chunk, materialising it in shared memory on first access. The interprocess
    /// mutex must be held by the caller.
    std::byte* getChunk(uint64_t bar, uint64_t chunkIndex);

    static void checkSizeIsMultipleOfWordSize(size_t sizeInBytes);

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
      // This must not be boost::thread since we don't want interruption points.
      // The problem is that boost::thread::join() is an interruption point for the calling thread. The
      // InterruptDispatcherThread does not rely on boost interruption points to control its tear down, it uses the
      // _stop flag for this purpose.
      std::thread _thr;
      std::atomic_bool _started{false};
      std::atomic_bool _stop{false};
    };
  };
} // namespace ChimeraTK
