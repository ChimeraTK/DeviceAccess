// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ProcessManagement.h"

#include <sys/types.h>

#include <pwd.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <string>

bool processExists(unsigned pid) {
  return !kill((pid_t)pid, 0);
}

unsigned getOwnPID() {
  return (unsigned)getpid();
}

std::string getUserName() {
  errno = 0;
  auto* pwent = getpwuid(geteuid());
  auto savedErr = errno;

  if(pwent == nullptr) {
    // pwent == nullptr and errno == 0 is a valid scenario - There was no error, but the system also
    // could not find the corresponding user name.
    if(savedErr != 0) {
      std::cout << "Failed to lookup user name, expect issues: " << strerror(savedErr) << std::endl;
    }
    return {"**unknown*user*name**"};
  }
  return {pwent->pw_name};
}
