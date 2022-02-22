#ifndef MTCA4U_SHARED_DUMMY_BACKEND_H
#define MTCA4U_SHARED_DUMMY_BACKEND_H

#include <list>
#include <map>
#include <mutex>
#include <set>
#include <utility>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/unordered_set.hpp>

#include "Exception.h"
#include "DummyBackendBase.h"
#include "ProcessManagement.h"
#include "RegisterInfoMap.h"

// Define shared-memory compatible vector type and corresponding allocator
typedef boost::interprocess::allocator<int32_t, boost::interprocess::managed_shared_memory::segment_manager>
    ShmemAllocator;
typedef boost::interprocess::vector<int32_t, ShmemAllocator> SharedMemoryVector;
typedef boost::interprocess::vector<pid_t, ShmemAllocator> PidSet;

namespace ChimeraTK {

  /** The shared dummy device opens a mapping file defining the registers and
   * implements them in shared memory instead of connecting to the real device.
   * Thus, it provides access to the registers from several applications. The
   * registers an application accesses can be stimulated or monitored by another
   * process, e.g. for development and testing.
   *
   *  Accessing applications are required to the same mapping file (matching
   * absolute path) and to be run by the same user.
   */
  class SharedDummyBackend : public DummyBackendBase<SharedDummyBackend> {
   public:
    SharedDummyBackend(std::string instanceId, std::string mapFileName);
    ~SharedDummyBackend() override;

    void open() override;
    void closeImpl() override;

    using DummyBackendBase<SharedDummyBackend>::read;  // use the 32 bit version from the base class
    using DummyBackendBase<SharedDummyBackend>::write; // use the 32 bit version from the base class
    void read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) override;
    void write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) override;

    std::string readDeviceInfo() override;
    bool isFunctional() const override { return (_opened && !_hasActiveException); }

    static boost::shared_ptr<DeviceBackend> createInstance(
        std::string address, std::map<std::string, std::string> parameters);

    VersionNumber triggerInterrupt(int interruptControllerNumber, int interruptNumber) override;

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
      SharedMemoryVector* findOrConstructVector(const std::string& objName, const size_t size);

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

      const size_t SHARED_MEMORY_N_MAX_MEMBER = 10;

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

      bool _reInitRequired = false;

      size_t getRequiredMemoryWithOverhead(void);
      void checkPidSetConsistency(void);
      void reInitMemory(void);
      std::vector<std::string> listNamedElements(void);

     protected:
      // interprocess mutex, has to be accessible by SharedDummyBackend class
      boost::interprocess::named_mutex interprocessMutex;

    }; /* class SharedMemoryManager */

    // Managed shared memory object
    SharedMemoryManager sharedMemoryManager;

    // Setup register bars in shared memory
    void setupBarContents();

    // Helper routines called in init list
    size_t getTotalRegisterSizeInBytes() const;

    static void checkSizeIsMultipleOfWordSize(size_t sizeInBytes);

    static std::string convertPathRelativeToDmapToAbs(std::string const& mapfileName);

   public:
    /** map of instance names and pointers to allow re-connecting to the same
     * instance with multiple Devices */
    static std::map<std::string, boost::weak_ptr<DeviceBackend>>& getInstanceMap() {
      static std::map<std::string, boost::weak_ptr<DeviceBackend>> instanceMap;
      return instanceMap;
    }
    // DummyBackendBase needs access to getInstanceMap()
    friend class DummyBackendBase<SharedDummyBackend>;
  };

} // namespace ChimeraTK

#endif // MTCA4U_SHARED_DUMMY_BACKEND_H
