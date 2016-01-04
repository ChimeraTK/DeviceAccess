#include <stdexcept>
#include "parserUtilities.h"
#include "Utilities.h"

std::string mtca4u::parserUtilities::getCurrentWorkingDirectory() {
  char *currentWorkingDir = get_current_dir_name();
  if (currentWorkingDir == NULL) {
    throw std::runtime_error("Could not get the current working directory");
  }
  std::string returnValue(currentWorkingDir);
  free(currentWorkingDir);
  return returnValue + "/";
}


std::string mtca4u::parserUtilities::combinePaths(std::string& absoluteBasePath,
                                         const std::string& pathToAppend) {
  std::string combinedPaths;
  if (absoluteBasePath.back() == '/') {
    combinedPaths = absoluteBasePath;
  } else {
    combinedPaths = absoluteBasePath + '/';
  }
  if (pathToAppend[0] == '/') { // absolute path, replace the base path
    combinedPaths = pathToAppend;
  } else { // relative path
    combinedPaths = combinedPaths + pathToAppend;
  }
  return combinedPaths;
}

std::string mtca4u::parserUtilities::getAbsolutePathToDirectory(
    const std::string& fileName) {
  std::string currentWorkingDir =
      getCurrentWorkingDirectory();
  size_t pos = fileName.find_last_of('/');
  if (pos == std::string::npos) { // No forward slashes; just the file name
    // so we return the absolute path of the current dir
    return currentWorkingDir;
  } else {
  		//
    return combinePaths(currentWorkingDir,
                        fileName.substr(0, pos + 1)); // include the /
  }
}

std::string mtca4u::parserUtilities::getAbsolutePathToFile(const std::string& fileName) {
  return getAbsolutePathToDirectory(fileName) +
         extractFileNameFromPath(fileName);
}


std::string mtca4u::parserUtilities::extractFileNameFromPath(
    const std::string& fileName) {
  std::string extractedName = fileName;
  size_t pos = fileName.find_last_of('/');
  if (pos != std::string::npos) {
    extractedName = fileName.substr(pos + 1, std::string::npos);
  }
  return extractedName;
}
