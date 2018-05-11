// Utilities to manage Linux processes

#include "ProcessManagement.h"
#include <boost/filesystem.hpp>
#include <unistd.h>
#include <string>

bool processExists(unsigned pid){
  
  boost::filesystem::path procdir = "/proc/" + std::to_string(pid);

  return boost::filesystem::exists(procdir) && boost::filesystem::is_directory(procdir);
  
}

unsigned getOwnPID(void){
  return (unsigned)getpid(); 
}


