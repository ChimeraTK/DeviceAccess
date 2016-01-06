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
  *
  * @return The absolute path to the file/directory, that had its relative path
  *         provided as the function parameter.
  */
  std::string convertToAbsolutePath(std::string const& relativePath);

  /*!
  * @brief Returns the absolute path to the directory containing the file
  * provided as the input parameter.
  *
  * Function also accepts relative paths. These are expected to be relative to
  * the programs current working directory. When the input is a directory name,
  * the method returns its absolute path. For a filename as input, it returns
  * the absolute path of the directory containing that file.
  *
  * @param path absolute/relative path to file/directory. Relative paths are
  *             expected to be relative to the programs current working
  *             directory.
  *
  * @return <ul>
  *           <li> The absolute path to the directory containing the file when
  *                the input is a path to a file.
  *           <li> The absolute path to the directory when the input is a path
  *                to a directory.
  *         </ul>
  */
  std::string getAbsolutePathToDirectory(std::string const& path);

  /*!
  * @brief Extract the string after the last '/' in a path.
  *
  *  The extracted substring from the input parameter path , does not  include
  *  the last '/' character.
  *
  * @param path string representing file/directory paths.
  *
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
  *
  * @return
  *        <ul>
  *          <li> path2 when the input parameter path2 is an absolute path.
  *          <li> path1 concatenated with path2 when path2 is a relative path.
  *        </ul>
  */
  std::string concatenatePaths(const std::string& path1, const std::string& path2);

}
}

#endif /* SOURCE_DIRECTORY__FILEPARSERS_SRC_PARSERUTILITIES_H_ */
