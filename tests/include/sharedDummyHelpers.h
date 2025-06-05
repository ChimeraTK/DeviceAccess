// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <sys/file.h>

#include <boost/interprocess/managed_shared_memory.hpp>

#include <iostream>
#include <string>
#include <utility>

enum class MirrorRequestType : int { from = 1, to, stop };

bool shm_exists(std::string shmName) {
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
