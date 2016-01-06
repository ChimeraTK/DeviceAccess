#include <stdexcept>
#include "parserUtilities.h"
#include "Utilities.h"

/*
 * Adds a '/' to the end of the input string path, only if path does not end in
 * a '/' character
 */
static std::string appendForwardSlash(const std::string & path);


std::string mtca4u::parserUtilities::getCurrentWorkingDirectory() {
  char* currentWorkingDir = get_current_dir_name();
  if (currentWorkingDir == nullptr) {
    throw std::runtime_error("Could not get the current working directory");
  }

  std::string returnValue(currentWorkingDir);
  free(currentWorkingDir);
  return appendForwardSlash(returnValue);
}

std::string mtca4u::parserUtilities::convertToAbsolutePath(
    const std::string& relativePath) {
  return (getAbsolutePathToDirectory(relativePath) +
          extractFileName(relativePath));
}

std::string mtca4u::parserUtilities::getAbsolutePathToDirectory(
    const std::string& path) {
  std::string currentWorkingDir = getCurrentWorkingDirectory();
  size_t pos = path.find_last_of('/');
  bool isPathJustFileName = (pos == std::string::npos);
  if (isPathJustFileName) { // No forward slashes in path; just the file name
    // meaning current dir has the file in it.
    return currentWorkingDir;
  } else {
    // we have either a relative/absolute path. extract text till last '/' from
    // path (/ included) and concatenate with cwd if path is relative
    return concatenatePaths(currentWorkingDir,
                        path.substr(0, pos + 1)); // include the /
  }
}

std::string mtca4u::parserUtilities::extractFileName(
    const std::string& path) {
  std::string extractedName = path;
  size_t pos = path.find_last_of('/');
  bool isPathJustFileName = (pos == std::string::npos); // no '/' in the string
  if (isPathJustFileName == false) {
    extractedName = path.substr(pos + 1, std::string::npos); // get the substring
                                                             // after the last '/' in the path
  }
  return extractedName;
}

std::string mtca4u::parserUtilities::concatenatePaths(
    const std::string& path1, const std::string& path2) {
  std::string returnValue;
  returnValue = appendForwardSlash(path1);

  if (path2[0] == '/') { // absolute path => return path2
    returnValue = path2;
  } else { // relative path
    returnValue = returnValue + path2;
  }
  return returnValue;
}

std::string appendForwardSlash(const std::string& path) {
  if (path.back() == '/') { // path ends with '/'
    return path;
  } else { // add '/' to path
    return path + "/";
  }
}
