// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "parserUtilities.h"

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <unistd.h>

namespace ChimeraTK::parserUtilities {

  /*
   * Adds a '/' to the end of the input string path, only if path does not end in
   * a '/' character
   */
  static std::string appendForwardSlash(const std::string& path);

  std::string getCurrentWorkingDirectory() {
    return appendForwardSlash(std::filesystem::current_path());
  }

  std::string convertToAbsolutePath(const std::string& relativePath) {
    return concatenatePaths(getCurrentWorkingDirectory(), relativePath);
  }

  std::string concatenatePaths(const std::string& path1, const std::string& path2) {
    std::string returnValue = path2;
    if(path2[0] != '/') { // path2 not absolute path
      returnValue = appendForwardSlash(path1) + path2;
    }
    return returnValue;
  }

  std::string extractDirectory(const std::string& path) {
    size_t pos = path.find_last_of('/');
    bool isPathJustFileName = (pos == std::string::npos);

    if(isPathJustFileName) { // No forward slashes in path; just the file name,
                             // meaning working directory has the file in it.
      return "./";
    }
    return path.substr(0, pos + 1); // substring till the last '/'. The '/'
                                    // character is included in the return
                                    // string
  }

  std::string extractFileName(const std::string& path) {
    std::string extractedName = path;

    size_t pos = path.find_last_of('/');
    bool isPathJustFileName = (pos == std::string::npos); // no '/' in the string

    if(!isPathJustFileName) {
      extractedName = path.substr(pos + 1, std::string::npos); // get the substring after the
                                                               // last '/' in the path
    }
    return extractedName;
  }

  std::string appendForwardSlash(const std::string& path) {
    if(path.empty()) {
      return "/";
    }
    if(path.back() == '/') { // path ends with '/'
      return path;
    }
    // add '/' to path
    return path + "/";
  }

} // namespace ChimeraTK::parserUtilities
