/*
 * HelperMethods.h
 *
 *  Created on: Dec 23, 2015
 *      Author: varghese
 */

#ifndef SOURCE_DIRECTORY__FILEPARSERS_SRC_HELPERMETHODS_H_
#define SOURCE_DIRECTORY__FILEPARSERS_SRC_HELPERMETHODS_H_

#include <string>

namespace mtca4u {
namespace helper { // TODO/FIXME Better names for file and namespace
  std::string combinePaths(std::string& absoluteBasePath, const std::string& pathToAppend);
  std::string getAbsolutePathToDirectory(std::string const&);
  std::string getAbsolutePathToFile(std::string const&);
  std::string extractFileNameFromPath(std::string const&);
}
}

#endif /* SOURCE_DIRECTORY__FILEPARSERS_SRC_HELPERMETHODS_H_ */
