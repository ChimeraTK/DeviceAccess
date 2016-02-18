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
#include <unordered_set>
#include <boost/shared_ptr.hpp>

// forward declaration
namespace xmlpp {
  class Node;
}

namespace mtca4u {

  class LogicalNameMap {

    public:

      /** Potential target types */
      enum TargetType { REGISTER, RANGE, CHANNEL, INT_CONSTANT, INT_VARIABLE };

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

          /** test if deviceName is set (depending on the targetType) */
          bool hasDeviceName() const {
            return targetType != TargetType::INT_CONSTANT && targetType != TargetType::INT_VARIABLE;
          }

          /** test if registerName is set (depending on the targetType) */
          bool hasRegisterName() const {
            return targetType != TargetType::INT_CONSTANT && targetType != TargetType::INT_VARIABLE;
          }

          /** test if firstIndex is set (depending on the targetType) */
          bool hasFirstIndex() const {
            return targetType == TargetType::RANGE;
          }

          /** test if length is set (depending on the targetType) */
          bool hasLength() const {
            return targetType == TargetType::RANGE;
          }

          /** test if channel is set (depending on the targetType) */
          bool hasChannel() const {
            return targetType == TargetType::CHANNEL;
          }

          /** test if value is set (depending on the targetType) */
          bool hasValue() const {
            return targetType == TargetType::INT_CONSTANT || targetType == TargetType::INT_VARIABLE;
          }

      };

      /** Constructor: parse map from XML file */
      LogicalNameMap(const std::string &fileName);

      /** Obtain register information for the named register. The register information can be updated, which will
       *  have effect on the logical map itself (and thus any other user of the same register info)! */
      boost::shared_ptr<RegisterInfo> getRegisterInfoShared(const std::string &name);

      /** Obtain register information for the named register (const version) */
      const RegisterInfo& getRegisterInfo(const std::string &name) const;

      /** Obtain list of all target devices referenced in the map */
      std::unordered_set<std::string> getTargetDevices() const;

    protected:

      std::string _fileName;

      /** actual register info map (register name to target information) */
      std::map< std::string,  boost::shared_ptr<RegisterInfo> > _map;

      /** throw a parsing error with more information */
      void parsingError(const std::string &message);

      /** extract a value from an XML subnode of the given node. This is a workaround for older libxml++ version
       *  which do not yet have support for xmlpp::Node::eval_to_string(). */
      std::string getValueFromXmlSubnode(const xmlpp::Node *node, const std::string &subnodeName);

  };

} // namespace mtca4u



#endif /* MTCA4U_LOGICAL_NAME_MAP_H */
