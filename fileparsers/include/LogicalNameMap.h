/*
 * LogicalNameMap.h
 *
 *  Created on: Feb 8, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_LOGICAL_NAME_MAP_H
#define MTCA4U_LOGICAL_NAME_MAP_H

#include <string>
#include <map>

namespace mtca4u {

  class LogicalNameMap {

    public:

      /** Potential target types */
      enum TargetType { REGISTER, RANGE, CHANNEL, INT_CONSTANT };

      /** Sub-class: single entry of the logical name mapping */
      class RegisterInfo {
        public:

          /** Type of the target */
          TargetType targetType;

          /** The target device alias */
          std::string deviceName;

          /** The target register name */
          std::string registerName;

          /** The first index in the range */
          unsigned int firstIndex;

          /** The length of the range (i.e. number of indices) */
          unsigned int length;

          /** The channel of the target 2D register */
          unsigned int channel;

          /** The constant integer value */
          int value;

      };

      /** Constructor: parse map from XML file */
      LogicalNameMap(const std::string &fileName);

      /** Obtain register information for the named register */
      const RegisterInfo& getRegisterInfo(const std::string &name);

    protected:

      std::string _fileName;

      /** actual register info map (register name to target information) */
      std::map<std::string, RegisterInfo> _map;

      /** throw a parsing error with more information */
      void parsingError(const std::string &message);

  };

} // namespace mtca4u



#endif /* MTCA4U_LOGICAL_NAME_MAP_H */
