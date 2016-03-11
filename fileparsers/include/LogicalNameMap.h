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

#include "BufferingRegisterAccessor.h"
#include "DeviceBackend.h"
#include "Value.h"
#include "RegisterPlugin.h"

// forward declaration
namespace xmlpp {
  class Node;
  class Element;
}

namespace mtca4u {

  class LogicalNameMap {

    public:

      /** Potential target types */
      enum TargetType { INVALID, REGISTER, RANGE, CHANNEL, INT_CONSTANT, INT_VARIABLE };

      /** Sub-class: single entry of the logical name mapping */
      class RegisterInfo {
        public:

          /** Type of the target */
          TargetType targetType;

          /** The target device alias */
          Value<std::string> deviceName;

          /** The target register name */
          Value<std::string> registerName;

          /** The first index in the range */
          Value<unsigned int> firstIndex;

          /** The length of the range (i.e. number of indices) */
          Value<unsigned int> length;

          /** The channel of the target 2D register */
          Value<unsigned int> channel;

          /** The constant integer value */
          Value<int> value;

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

          /** create the internal register accessors */
          void createInternalAccessors(boost::shared_ptr<DeviceBackend> &backend) {
            deviceName.createInternalAccessors(backend);
            registerName.createInternalAccessors(backend);
            firstIndex.createInternalAccessors(backend);
            length.createInternalAccessors(backend);
            channel.createInternalAccessors(backend);
            value.createInternalAccessors(backend);
          }

          /** constuctor: initialise values */
          RegisterInfo()
          : targetType(TargetType::INVALID)
          {}

          /** Obtain a potentially modified buffering register accessor from the given accessor. Any plugins specified
           *  in the map for this register might modify the accessor. */
          template<typename UserType>
          boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > getBufferingRegisterAccessor(
              boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > accessor) const {
            for(auto i = pluginList.begin(); i != pluginList.end(); ++i) {
              accessor = (*i)->getBufferingRegisterAccessor<UserType>(accessor);
            }
            return accessor;
          }

          /** Obtain a potentially modified (non-buffering) register accessor from the given accessor. Any plugins
           *  specified in the map for this register might modify the accessor. */
          boost::shared_ptr<RegisterAccessor> getRegisterAccessor(boost::shared_ptr<RegisterAccessor> accessor) const {
            for(auto i = pluginList.begin(); i != pluginList.end(); ++i) {
              accessor = (*i)->getRegisterAccessor(accessor);
            }
            return accessor;
          }

        protected:

          /** list of plugins */
          std::vector< boost::shared_ptr<RegisterPlugin> > pluginList;

          friend class LogicalNameMap;
      };

      /** Class to store a register path */
      class RegisterPath {
        public:
          RegisterPath() : path(separator) {}
          RegisterPath(const std::string &_path) : path(separator+_path) {removeExtraSeparators();}
          RegisterPath(const RegisterPath &_path) : path(_path.path) {removeExtraSeparators();}

          /** type conversion operators into std::string */
          operator const std::string&() const { return path; }

          /** / operator: add a new element to the path (without modifying this object) */
          RegisterPath operator/(const std::string &rightHandSide) const {
            return RegisterPath(path+separator+rightHandSide);
          }
          RegisterPath operator/(const RegisterPath &rightHandSide) const {
            return RegisterPath(path+separator+rightHandSide.path);
          }

          /** /= operator: modify this object by adding a new element to this path */
          RegisterPath& operator/=(const std::string &rightHandSide) {
            path += separator+rightHandSide;
            removeExtraSeparators();
            return *this;
          }

          /** += operator: just concatenate-assign like normal strings */
          RegisterPath& operator+=(const std::string &rightHandSide) {
            path += rightHandSide;
            return *this;
          }

          /** Post-decrement operator, e.g.: registerPath--
           *  Remove the last element from the path */
          RegisterPath& operator--(int) {
            std::size_t found = path.find_last_of(separator);
            if(found != std::string::npos && found > 0) {               // don't find the leading separator...
              path = path.substr(0,found);
            }
            else {
              path = separator;
            }
            return *this;
          }

          /** Pre-decrement operator, e.g.: --registerPath
           *  Remove the first element form the path */
          RegisterPath& operator--() {
            std::size_t found = path.find_first_of(separator,1);        // don't find the leading separator...
            if(found != std::string::npos) {
              path = separator + path.substr(found+1);
            }
            else {
              path = separator;
            }
            return *this;
          }

          // comparison with other RegisterPath
          bool operator==(const RegisterPath &rightHandSide) const {
            return *this == rightHandSide.path;
          }

          // comparison with std::string
          bool operator==(const std::string &rightHandSide) const {
            RegisterPath temp(rightHandSide);
            return path == temp.path;
          }

          // comparison with char*
          bool operator==(const char *rightHandSide) const {
            RegisterPath temp(rightHandSide);
            return path == temp.path;
          }

        protected:

          std::string path;
          static const char separator[];

          /** Search for duplicate separators (e.g. "//") and remove one of them. Also removes a trailing separator,
           *  if present. */
          void removeExtraSeparators() {
            std::size_t pos;
            while( (pos = path.find(std::string(separator)+separator)) != std::string::npos ) {
              path.erase(pos,1);
            }
            if(path.length() > 1 && path.substr(path.length()-1,1) == separator) path.erase(path.length()-1,1);
          }
      };

      /** Constructor: parse map from XML file */
      LogicalNameMap(const std::string &fileName) {
        parseFile(fileName);
      }

      /** Default constructor: Create empty map. */
      LogicalNameMap() {
      }

      /** Obtain register information for the named register. The register information can be updated, which *will*
       *  have an effect on the logical map itself (and thus any other user of the same register info)! */
      boost::shared_ptr<RegisterInfo> getRegisterInfoShared(const std::string &name);

      /** Obtain register information for the named register (const version) */
      const RegisterInfo& getRegisterInfo(const std::string &name) const;

      /** Obtain list of all target devices referenced in the map */
      std::unordered_set<std::string> getTargetDevices() const;

    protected:

      /** file name of the logical map */
      std::string _fileName;

      /** current register path in the map */
      RegisterPath currentModule;

      /** actual register info map (register name to target information) */
      std::map< std::string, boost::shared_ptr<RegisterInfo> > _map;

      /** parse the given XML file */
      void parseFile(const std::string &fileName);

      /** called inside parseFile() to parse an XML element and its sub-elements recursivly */
      void parseElement(RegisterPath currentPath, const xmlpp::Element *element);

      /** throw a parsing error with more information */
      void parsingError(const std::string &message);

      /** Build a Value object for a given subnode. */
      template<typename ValueType>
      Value<ValueType> getValueFromXmlSubnode(const xmlpp::Node *node, const std::string &subnodeName);

  };

  /** non-member + operator for RegisterPath: just concatenate like normal strings */
  std::string operator+(const LogicalNameMap::RegisterPath &leftHandSide, const std::string &rightHandSide);
  std::string operator+(const std::string &leftHandSide, const LogicalNameMap::RegisterPath &rightHandSide);
  std::string operator+(const LogicalNameMap::RegisterPath &leftHandSide, const LogicalNameMap::RegisterPath &rightHandSide);

  /** non-member / operator: add a new element to the path from the front */
  LogicalNameMap::RegisterPath operator/(const std::string &leftHandSide, const LogicalNameMap::RegisterPath &rightHandSide);

} // namespace mtca4u

#endif /* MTCA4U_LOGICAL_NAME_MAP_H */
