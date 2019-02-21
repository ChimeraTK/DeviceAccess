/*
 * Utilities.h
 *
 *  Created on: Aug 3, 2015
 *      Author: nshehzad
 */

#ifndef CHIMERA_TK_UTILITES_H
#define CHIMERA_TK_UTILITES_H

#include <list>
#include <map>

#include "DeviceInfoMap.h"
#include "Exception.h"

namespace ChimeraTK {

///
/// @brief Returns the dmap file name which the library currently uses for
/// looking up device(alias) names.
///
std::string getDMapFilePath();

///
/// @brief Set the location of the dmap file. The library will parse this dmap
/// file for the device(alias) lookup.
/// @param dmapFilePath Relative or absolute path of the dmap file (directory
/// and file name).
///
///
void setDMapFilePath(std::string dmapFilePath);

/**
 *  This structure holds the information of an ChimeraTK device descriptor.
 */
struct DeviceDescriptor {
  std::string backendType;
  std::string address;
  std::map<std::string, std::string> parameters;
};

/**
 *  This structure holds the information of an SDM.
 */
struct Sdm {
  double _SdmVersion;
  std::string _Host;
  std::string _Interface;
  std::string _Instance;
  std::string _Protocol;
  std::list<std::string> _Parameters;
  Sdm() : _SdmVersion(0.1) {}
};

/**
 *  A class to provide generic useful function accross the library."
 */
class Utilities {
public:
  Utilities(){};

  /** Parse a ChimeraTK device descriptor (CDD) and return the information in
   * the DeviceDescriptor struct */
  DeviceDescriptor static parseDeviceDesciptor(std::string cddString);

  /** Check whether the given string seems to be a CDD. There is no guarantee
   * that the CDD is well-formed, the
   *  function just looks for the signature of a CDD. */
  bool static isDeviceDescriptor(std::string theString);

  /** Parse an SDM URI and return the device information in the Sdm struct. */
  Sdm static parseSdm(std::string sdmString);

  /** Parse an old-style device string (either path to device node or map file
   * name for dummies) */
  Sdm static parseDeviceString(std::string deviceEntry);

  /** Check wehter the given string seems to be an SDM. There is no guarantee
   * that the SDM is well-formed, the finction just looks for the signatore of
   * an SDM. */
  bool static isSdm(std::string theString);

  /** Check if the given string only contains alphanumeric characters */

  static size_t countOccurence(std::string theString, char delimiter);

  /// Search for an alias in a given DMap file and return the DeviceInfo entry.
  /// If the alias is not found, the DeviceInfo will have empty strings.
  DeviceInfoMap::DeviceInfo static aliasLookUp(std::string aliasName,
                                               std::string dmapFilePath);

  /// Returns the list of device aliases from the DMap file set using
  /// @ref BackendFactory::setDMapFilePath
  static std::vector<std::string> getAliasList();

  /// Print a call stack trace (but continue executing the process normally).
  /// Can be used for debugging. C++ names will be demangled, if possible.
  static void printStackTrace();
};

} /* namespace ChimeraTK */

#endif /* CHIMERA_TK_UTILITES_H */
