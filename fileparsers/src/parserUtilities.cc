#include <stdexcept>
#include <unistd.h>
#include "parserUtilities.h"

/*
 * Adds a '/' to the end of the input string path, only if path does not end in
 * a '/' character
 */
static std::string appendForwardSlash(const std::string& path);

std::string mtca4u::parserUtilities::getCurrentWorkingDirectory() {
  char* currentWorkingDir = get_current_dir_name();
  if (currentWorkingDir == nullptr) {
    throw std::runtime_error("Could not get the current working directory");
  }
  std::string returnValue(currentWorkingDir);
  free(currentWorkingDir);
  // append '/' to the end if not present and return
  return appendForwardSlash(returnValue);
}

std::string mtca4u::parserUtilities::convertToAbsolutePath(
    const std::string& relativePath) {
  return concatenatePaths(getCurrentWorkingDirectory(), relativePath);
}

std::string mtca4u::parserUtilities::concatenatePaths(
    const std::string& path1, const std::string& path2) {
  std::string returnValue = path2;
  if (path2[0] != '/') { // path2 not absolute path 
    returnValue = appendForwardSlash(path1) + path2;
  }
  return returnValue;
}

std::string mtca4u::parserUtilities::extractDirectory(const std::string& path) {
  size_t pos = path.find_last_of('/');
  bool isPathJustFileName = (pos == std::string::npos);

  if (isPathJustFileName) { // No forward slashes in path; just the file name,
	                    // meaning working directory has the file in it.
    return "./";
  } else {
    return path.substr(0, pos + 1); // substring till the last '/'. The '/'
                                    // character is included in the return
                                    // string
  }
}

std::string mtca4u::parserUtilities::extractFileName(const std::string& path) {
  std::string extractedName = path;

  size_t pos = path.find_last_of('/');
  bool isPathJustFileName = (pos == std::string::npos); // no '/' in the string

  if (isPathJustFileName == false) {
    extractedName =
        path.substr(pos + 1, std::string::npos); // get the substring after the
                                                 // last '/' in the path
  }
  return extractedName;
}

std::string appendForwardSlash(const std::string& path) {
  if (path.back() == '/') { // path ends with '/'
    return path;
  } else { // add '/' to path
    return path + "/";
  }
}

