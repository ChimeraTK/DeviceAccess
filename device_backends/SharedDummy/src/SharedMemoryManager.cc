// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SharedDummyBackend.h"

namespace ChimeraTK {

  // Construct/deconstruct
  SharedDummyBackend::SharedMemoryManager::SharedMemoryManager(
      SharedDummyBackend& sharedDummyBackend_, const std::string& instanceId, const std::string& mapFileName)
  : sharedDummyBackend(sharedDummyBackend_), userHash(std::to_string(std::hash<std::string>{}(getUserName()))),
    mapFileHash(std::to_string(std::hash<std::string>{}(mapFileName))),
    instanceIdHash(std::to_string(std::hash<std::string>{}(instanceId))),
    name("ChimeraTK_SharedDummy_" + instanceIdHash + "_" + mapFileHash + "_" + userHash),
    segment(boost::interprocess::open_or_create, name.c_str(), getRequiredMemoryWithOverhead()),
    sharedMemoryIntAllocator(segment.get_segment_manager()),
    interprocessMutex(boost::interprocess::open_or_create, name.c_str()) {
    {
      // lock guard with the interprocess mutex
      std::lock_guard<boost::interprocess::named_mutex> lock(interprocessMutex);

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
    }
    this->intDispatcherIf = boost::movelib::unique_ptr<InterruptDispatcherInterface>(
        new InterruptDispatcherInterface(sharedDummyBackend, segment, interprocessMutex));
  }

  SharedDummyBackend::SharedMemoryManager::~SharedMemoryManager() {
    // stop and delete dispatcher thread first since it uses shm and mutex
    intDispatcherIf.reset();
    int pidSetSize;
    {
      // lock guard with the interprocess mutex
      std::lock_guard<boost::interprocess::named_mutex> lock(interprocessMutex);

      // Clean up
      checkPidSetConsistency();

      int32_t ownPid = static_cast<int32_t>(getOwnPID());
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
    // If size of pidSet is 0 (i.e, the instance belongs to the last accessing
    // process), destroy shared memory and the interprocess mutex
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

    if(pidSetSizeBeforeCleanup != 0 && pidSet->size() == 0) {
      return true;
    }
    return false;
  }

  void SharedDummyBackend::SharedMemoryManager::reInitMemory() {
    std::vector<std::string> nameList = listNamedElements();

    for(auto item = nameList.begin(); item != nameList.end(); ++item) {
      if(item->compare(SHARED_MEMORY_REQUIRED_VERSION_NAME) == 0) {
        segment.destroy<unsigned>(item->c_str());
      }
      // reset the BAR vectors in shm.
      // Note, InterruptDispatcherInterface uses unique_instance mechanism so it is not affected here
      else if(item->compare(SHARED_MEMORY_PID_SET_NAME) != 0) {
        segment.destroy<SharedMemoryVector>(item->c_str());
      }
    }
    InterruptDispatcherInterface::cleanupShm(segment);
  }

  std::vector<std::string> SharedDummyBackend::SharedMemoryManager::listNamedElements() {
    std::vector<std::string> list;

    for(auto seg = segment.named_begin(); seg != segment.named_end(); ++seg) {
      list.push_back(seg->name());
    }
    return list;
  }

} /* namespace ChimeraTK */
