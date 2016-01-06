#ifndef SOURCE_DIRECTORY__FILEPARSERS_SRC_PARSERUTILITIES_H_
#define SOURCE_DIRECTORY__FILEPARSERS_SRC_PARSERUTILITIES_H_

#include <string>

namespace mtca4u {
//! common methods for dmap file parsing
namespace parserUtilities {

  /*!
  * @brief Returns absolute path to current working directory.
  * The returned path terminates in a forward slash.
  *
  * @return Current working directory of the program terminating in a
  *         forward slash.
  */
  std::string getCurrentWorkingDirectory();

  /*!
  * @brief Converts a relative path to its absolute path.
  *
  * Method converts a path relative to the programs current working directory
  * to an absolute path. Providing an absolute path as parameter, still returns
  * the absolute path.
  *
  * @param relativePath Path to a file/directory relative to the programs
  *                     current working directory.
  * @return The absolute path to the file/directory, that had its relative path
  *         provided as the function parameter.
  */
  std::string convertToAbsolutePath(std::string const& relativePath);

  /*!
  * @brief Returns the path to the directory containing the file
  * provided as the input parameter.
  *
  * @param path path to a file/directory.
  * @return <ul>
  *           <li> Path to the directory containing the file when
  *                the input is a path to a file. (i.e path does not end in a
  *                '/')
  *           <li> Path to the directory when the input is a path
  *                to a directory. (i.e input path ends in a '/')
  *         </ul>
  */
  std::string extractDirectory(std::string const& path);

  /*!
  * @brief Extract the string after the last '/' in a path. Returned substring
  * does not include the '/' character.
  *
  * @param path string representing file/directory paths.
  * @return substring containing characters after the last '/' in the input
  *                   string. The '/' is excluded in this substring.
  */
  std::string extractFileName(std::string const& path);

  /*!
  * @brief Concatenates two given paths using custom rules.
  *
  * Method returns path2 when input parameter path2 is an absolute
  * path. Otherwise path1 is concatenated with path2 and returned.
  *
  * @param path1, path2 strings representing file/directory paths.
  * @return
  *        <ul>
  *          <li> path2 when the input parameter path2 is an absolute path.
  *          <li> path1 concatenated with path2 when path2 is a relative path.
  *        </ul>
  */
  std::string concatenatePaths(const std::string& path1,
                               const std::string& path2);
}
}

#endif /* SOURCE_DIRECTORY__FILEPARSERS_SRC_PARSERUTILITIES_H_ */
