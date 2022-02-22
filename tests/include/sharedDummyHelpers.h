#include <string>
#include <utility>
#include <iostream>
#include <boost/interprocess/managed_shared_memory.hpp>

#include <sys/file.h>

enum MirrorRequest_Type { mirrorRequest_From = 1, mirrorRequest_To, mirrorRequest_Stop };

// Static helper functions
static std::string createExpectedShmName(std::string instanceId_, std::string mapFileName_, std::string userName) {
  std::string mapFileHash{std::to_string(std::hash<std::string>{}(mapFileName_))};
  std::string instanceIdHash{std::to_string(std::hash<std::string>{}(instanceId_))};
  std::string userHash{std::to_string(std::hash<std::string>{}(userName))};

  return "ChimeraTK_SharedDummy_" + instanceIdHash + "_" + mapFileHash + "_" + userHash;
}

static bool shm_exists(std::string shmName) {
  bool result;

  try {
    boost::interprocess::managed_shared_memory shm{boost::interprocess::open_only, shmName.c_str()};
    result = shm.check_sanity();
  }
  catch(const std::exception& ex) {
    result = false;
  }
  return result;
}

// Use a file lock on dmap-file to ensure we are not running
// concurrent tests in parallel using the same shared dummies.
//
// Note: flock() creates an advisory lock only, plain file access is not
// prevented. The lock is automatically released when the process terminates!
struct TestLocker {
  TestLocker(const char* dmapFile) {
    // open dmap file for locking
    fd = open(dmapFile, O_RDONLY);
    if(fd == -1) {
      std::cout << "Cannot open file '" << dmapFile << "' for locking." << std::endl;
      exit(1);
    }

    // obtain lock
    int res = flock(fd, LOCK_EX);
    if(res == -1) {
      std::cout << "Cannot acquire lock on file '" << dmapFile << "'." << std::endl;
      exit(1);
    }
  }

  int fd;
};
