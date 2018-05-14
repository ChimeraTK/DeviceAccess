// Utilities to manage Linux processes

#include "ProcessManagement.h"
//#include <boost/filesystem.hpp>
#include <unistd.h>
#include <string>
#include <signal.h>
#include <sys/types.h>

bool processExists(unsigned pid){
  
  return !kill((pid_t)pid, 0);
  //boost::filesystem::path procdir = "/proc/" + std::to_string(pid);

  //return boost::filesystem::exists(procdir) && boost::filesystem::is_directory(procdir);
  
}

unsigned getOwnPID(void){
  return (unsigned)getpid(); 
}


