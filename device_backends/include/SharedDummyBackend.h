#ifndef MTCA4U_SHARED_DUMMY_BACKEND_H
#define MTCA4U_SHARED_DUMMY_BACKEND_H

#include <vector>
#include <map>
#include <list>
#include <set>
#include <mutex>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/function.hpp>

#include "Exception.h"
#include "RegisterInfoMap.h"
#include "NumericAddressedBackend.h"

namespace mtca4u {

  /** TODO DOCUMENTATION
   */
  class SharedDummyBackend : public NumericAddressedBackend
  {
    public:
      SharedDummyBackend(std::string instanceId, std::string mapFileName);
      virtual ~SharedDummyBackend();

      virtual void open();
      virtual void close();
      virtual void read(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes);
      virtual void write(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes);
      virtual std::string readDeviceInfo();
      
      int32_t& getRegisterContent(uint8_t bar, uint32_t address);

      static boost::shared_ptr<DeviceBackend> createInstance(std::string host, std::string instance,
                                                            std::list<std::string> parameters, std::string mapFileName);

    private:

      /** name of the map file */
      std::string _mapFile;

      RegisterInfoMapPointer _registerMapping;

      // Define shared-memory compatible vector type
      typedef boost::interprocess::allocator<int32_t, boost::interprocess::managed_shared_memory::segment_manager>  ShmemAllocator;
      typedef boost::interprocess::vector<int32_t, ShmemAllocator> MyVector;
      
      // Bar contents with shared-memory compatible vector type. Plain pointers are used here since this is what we
      // get from the shared memory allocation.
      std::map<uint8_t, MyVector*> _barContents;
      
      // bar sizes in words
      std::map<uint8_t, size_t> _barSizes;
      

      // helper class to manage the shared memory: automatically construct if necessary, automatically destroy if
      // last using process closes.
      struct MyShm {
        
        MyShm() {}

        void setup(const std::string &name, size_t maxSize) {
          
          _name = name;
          
          segment = {boost::interprocess::open_or_create, name.c_str(), maxSize};

          // lock gurad with the interprocess mutex        
          std::lock_guard<boost::interprocess::named_mutex> lock(globalMutex);
        
          // find the use counter
          auto res = segment.find<size_t>("UseCounter");
          if(res.second != 1) {  // if not found: create it
            useCount = segment.construct<size_t>("UseCounter")(0);
          }
          else {
            useCount = res.first;
          }
          
          // increment use counter
          (*useCount)++;
          
        }

        ~MyShm() {

          // lock gurad with the interprocess mutex        
          std::lock_guard<boost::interprocess::named_mutex> lock(globalMutex);
          
          // decrement use counter
          (*useCount)--;
          
          // if use count at 0, destroy shared memory and the interprocess mutex
          if(*useCount == 0) {
            boost::interprocess::shared_memory_object::remove(_name.c_str());
            boost::interprocess::named_mutex::remove(_name.c_str());
          }
        }
        
        // the name of the segment
        std::string _name;

        // the shared memory segment
        boost::interprocess::managed_shared_memory segment;

        // the allocator instance
        const ShmemAllocator alloc_inst{segment.get_segment_manager()};
        
        // global (interprocess) mutex
        boost::interprocess::named_mutex globalMutex{boost::interprocess::open_or_create, _name.c_str()};

        // pointer to the use count on shared memory;
        size_t *useCount{nullptr};

      };
      
      // define our shared memory
      MyShm shm;
      
      // shared memory name
      std::string shmName;


      static void checkSizeIsMultipleOfWordSize(size_t sizeInBytes);

      static std::string convertPathRelativeToDmapToAbs(std::string const & mapfileName);

      /** map of instance names and pointers to allow re-connecting to the same instance with multiple Devices */
      static std::map< std::string, boost::shared_ptr<mtca4u::DeviceBackend> >& getInstanceMap() {
        static std::map< std::string, boost::shared_ptr<mtca4u::DeviceBackend> > instanceMap;
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
      template <typename T, typename... Args>
      static boost::shared_ptr<DeviceBackend> returnInstance(const std::string& instanceId, Args&&... arguments) {
        if (instanceId == "") {
          // std::forward because template accepts forwarding references
          // (Args&&) and this can have both lvalue and rvalue references passed
          // as arguments.
          return boost::shared_ptr<DeviceBackend>(new T(std::forward<Args>(arguments)...));
        }
        // search instance map and create new instanceId, if not found under the
        // name
        if (getInstanceMap().find(instanceId) == getInstanceMap().end()) {
          boost::shared_ptr<DeviceBackend> ptr(new T(std::forward<Args>(arguments)...));
          getInstanceMap().insert(std::make_pair(instanceId, ptr));
          return ptr;
        }
        // return existing instanceId from the map
        return boost::shared_ptr<DeviceBackend>(getInstanceMap()[instanceId]);
      }

  };

} // namespace mtca4u

#endif // MTCA4U_SHARED_DUMMY_BACKEND_H
