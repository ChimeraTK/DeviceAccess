// Utilities to manage Linux processes

#include "ProcessManagement.h"
#include <signal.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>

bool processExists(unsigned pid) {
  return !kill((pid_t)pid, 0);
}

unsigned getOwnPID(void) {
  return (unsigned)getpid();
}

std::string getUserName(void) {
  auto* login = getlogin();
  if(login == nullptr) {
    return "**unknown*user*name**";
  }
  return std::string(login);
}
