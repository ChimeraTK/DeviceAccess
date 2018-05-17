// Utilities to manage Linux processes
#include <string>

bool processExists(unsigned pid);
unsigned getOwnPID(void);
std::string getUserName(void);
