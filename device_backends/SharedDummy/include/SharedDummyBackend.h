#ifndef MTCA4U_SHARED_DUMMY_BACKEND_H
#define MTCA4U_SHARED_DUMMY_BACKEND_H

#include <vector>
#include <map>
#include <list>
#include <set>
#include <mutex>
#include <utility>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/unordered_set.hpp>
#include <boost/function.hpp>
#include <boost/filesystem.hpp>

#include "Exception.h"
#include "RegisterInfoMap.h"
#include "NumericAddressedBackend.h"
#include "ProcessManagement.h"

// Define shared-memory compatible vector type and corresponding allocator
typedef boost::interprocess::allocator<int32_t, boost::interprocess::managed_shared_memory::segment_manager>
    ShmemAllocator;
typedef boost::interprocess::vector<int32_t, ShmemAllocator> SharedMemoryVector;
typedef boost::interprocess::vector<int32_t, ShmemAllocator> PidSet;

namespace ChimeraTK {

  /** The shared dummy device opens a mapping file defining the registers and implements
   *  them in shared memory instead of connecting to the real device. Thus, it provides access
   *  to the registers from several applications. The registers an application accesses can be
   *  stimulated or monitored by another process, e.g. for development and testing.
   *
   *  Accessing applications are required to the same mapping file (matching absolute path) and
   *  to be run by the same user.
   */
  class SharedDummyBackend : public NumericAddressedBackend {
   public:
    SharedDummyBackend(std::string instanceId, std::string mapFileName);
    virtual ~SharedDummyBackend();

    virtual void open();
    virtual void close();
    virtual void read(uint8_t bar, uint32_t address, int32_t* data, size_t sizeInBytes);
    virtual void write(uint8_t bar, uint32_t address, int32_t const* data, size_t sizeInBytes);
    virtual std::string readDeviceInfo();

    int32_t& getRegisterContent(uint8_t bar, uint32_t address);

    static boost::shared_ptr<DeviceBackend> createInstance(
        std::string address, std::map<std::string, std::string> parameters);

   private:
    /** name of the map file */
    std::string _mapFile;

    RegisterInfoMapPointer _registerMapping;

    // Bar contents with shared-memory compatible vector type. Plain pointers are used here since this is what we
    // get from the shared memory allocation.
    std::map<uint8_t, SharedMemoryVector*> _barContents;

    // Bar sizes
    std::map<uint8_t, size_t> _barSizesInBytes;

    // Naming of bars as shared memory elements
    const char* SHARED_MEMORY_BAR_PREFIX = "BAR_";

    // Helper class to manage the shared memory: automatically construct if necessary,
    // automatically destroy if last using process closes.
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
       * @retval std::pair<size_t, size_t> first: Size of the memory segment, second: free memory in segment
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

    void setupBarContents();
    std::map<uint8_t, size_t> getBarSizesInBytesFromRegisterMapping() const;

    // Helper routines called in init list
    size_t getTotalRegisterSizeInBytes() const;

    static void checkSizeIsMultipleOfWordSize(size_t sizeInBytes);

    static std::string convertPathRelativeToDmapToAbs(std::string const& mapfileName);

    /** map of instance names and pointers to allow re-connecting to the same instance with multiple Devices */
    static std::map<std::string, boost::shared_ptr<DeviceBackend>>& getInstanceMap() {
      static std::map<std::string, boost::shared_ptr<DeviceBackend>> instanceMap;
      return instanceMap;
    }

    /**
     * @brief Method looks up and returns an existing instance of class 'T'
     * corresponding to instanceId, if instanceId is a valid  key in the
     * internal map. For an instanceId not in the internal map, a new instance
     * of class T is created, cached and returned. Future calls to
     * returnInstance with this instanceId, returns this cached instance. If
     * the instanceId is "" a new instance of class T is created and
     * returned. This instance will not be cached in the internal memory.
     *
     * @param instanceId Used as key for the object instance look up. "" as
     *                   instanceId will return a new T instance that is not
     *                   cached.
     * @param arguments  This is a template argument list. The constructor of
     *                   the created class T, gets called with the contents of
     *                   the argument list as parameters.
     */
    template<typename T, typename... Args>
    static boost::shared_ptr<DeviceBackend> returnInstance(const std::string& instanceId, Args&&... arguments) {
      if(instanceId == "") {
        // std::forward because template accepts forwarding references
        // (Args&&) and this can have both lvalue and rvalue references passed
        // as arguments.
        return boost::shared_ptr<DeviceBackend>(new T(std::forward<Args>(arguments)...));
      }
      // search instance map and create new instanceId, if not found under the
      // name
      if(getInstanceMap().find(instanceId) == getInstanceMap().end()) {
        boost::shared_ptr<DeviceBackend> ptr(new T(std::forward<Args>(arguments)...));
        getInstanceMap().insert(std::make_pair(instanceId, ptr));
        return ptr;
      }
      // return existing instanceId from the map
      return boost::shared_ptr<DeviceBackend>(getInstanceMap()[instanceId]);
    }
  };

} // namespace ChimeraTK

#endif // MTCA4U_SHARED_DUMMY_BACKEND_H
