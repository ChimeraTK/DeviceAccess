#ifndef SOURCE_DIRECTORY__FILEPARSERS_SRC_PARSERUTILITIES_H_
#define SOURCE_DIRECTORY__FILEPARSERS_SRC_PARSERUTILITIES_H_

#include <string>

namespace mtca4u {
//! common methods for dmap file parsing
namespace parserUtilities {

  /*!
  * @brief Returns absolute path of current working directory.
  * The returned path terminates with a forward slash.
  *
  * @return Current working directory of the program terminating in a
  *         forward slash.
  */
  std::string getCurrentWorkingDirectory();

  std::string combinePaths(std::string& absoluteBasePath,
                           const std::string& pathToAppend);
  std::string getAbsolutePathToDirectory(std::string const&);
  std::string getAbsolutePathToFile(std::string const&);
  std::string extractFileNameFromPath(std::string const&);
}
}

#endif /* SOURCE_DIRECTORY__FILEPARSERS_SRC_PARSERUTILITIES_H_ */
