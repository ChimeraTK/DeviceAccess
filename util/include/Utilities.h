/*
 * Utilities.h
 *
 *  Created on: Aug 3, 2015
 *      Author: nshehzad
 */

#ifndef CHIMERA_TK_UTILITES_H
#define CHIMERA_TK_UTILITES_H

#include <list>

#include "Exception.h"
#include "DeviceInfoMap.h"

namespace ChimeraTK {

///
/// @brief Returns the dmap file name which the library currently uses for
/// looking up device(alias) names.
///
std::string getDMapFilePath();

///
/// @brief Set the location of the dmap file. The library will parse this dmap
/// file for the device(alias) lookup.
/// @param dmapFilePath Relative or absolute path of the dmap file (directory and file name).
///                     
///
void setDMapFilePath(std::string dmapFilePath);

  /**This structure holds the information of an SDM.
   *
   */
  struct Sdm
  {
      double _SdmVersion;
      std::string _Host;
      std::string _Interface;
      std::string _Instance;
      std::string _Protocol;
      std::list<std::string> _Parameters;
      Sdm():_SdmVersion(0.1){}
  };
  /** A dedicated exception for the sdm parser."
   *
   */
  class SdmUriParseException : public Exception {
    public:
      enum {INVALID_SDM};
      SdmUriParseException(const std::string &message, unsigned int exceptionID)
      : Exception( message, exceptionID ){}
  };

  /** A class to provide generic useful function accross the library."
   *
   */
  class Utilities {

    public:
      Utilities(){};
      Sdm static parseSdm(std::string sdmString);
      Sdm static parseDeviceString(std::string deviceEntry);
      bool static isSdm(std::string theString);
      static size_t countOccurence(std::string theString, char delimiter);

      /// Search for an alias in a given DMap file and return the DeviceInfo entry.
      /// If the alias is not found, the DeviceInfo will have empty strings.
      DeviceInfoMap::DeviceInfo static aliasLookUp(std::string aliasName, std::string dmapFilePath);

      /// Search for an alias in all possible dmap file.
      /// The return value is the DeviceInfo where the alias was found (also
      /// containg the DMap file where the entry was found)
      // DeviceInfoMap::DeviceInfo static findFirstOfAlias(std::string aliasName);
      std::string static findFirstOfAlias(std::string aliasName);

      /// Returns the list of device aliases from the DMap file set using
      /// @ref BackendFactory::setDMapFilePath
      static std::vector<std::string> getAliasList();
      
      /// Print a call stack trace (but continue executing the process normally). Can be used for debugging. C++
      /// names will be demangled, if possible.
      static void printStackTrace();

  };

} /* namespace ChimeraTK */

#endif /* CHIMERA_TK_UTILITES_H */
