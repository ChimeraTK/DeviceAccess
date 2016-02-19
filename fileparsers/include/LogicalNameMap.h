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

// forward declaration
namespace xmlpp {
  class Node;
  class Element;
}

namespace mtca4u {

  class LogicalNameMap {

    public:
      // forward declaration
      class RegisterInfo;

      /** Sub-class: value of a RegisterInfo field (with proper resolution of dynamic references) */
      template<typename ValueType>
      class Value {

        public:

          // obtain value via implicit type conversion operator
          operator const ValueType&() const {
            if(!hasActualValue) {
              if(!accessor) {
                if(!backend) throw DeviceException("Cannot obtain this value without an associated device.",
                    DeviceException::EX_NOT_OPENED);
                accessor = backend->getBufferingRegisterAccessor<ValueType>("",registerName);
              }
              accessor->read();
              return (*accessor)[0];
            }
            return value;
          }

          // assignment operator with a value of the type ValueType. This will make the Value having an "actual"
          // value, i.e. no dynamic references are present.
          Value<ValueType>& operator=(const ValueType &rightHandSide) {
            value = rightHandSide;
            hasActualValue = true;
            return *this;
          }

          // allows comparisons with char* if ValueType is std::string
          bool operator==(const char *rightHandSide) {
            return value == rightHandSide;
          }

          // default constructor: no backend will be initialised, so no accessors can be created. It may not be
          // possible to obtain the value like this!
          Value()
          : hasActualValue(true), backend()
          {}

          // assignment operator: allow assigment to Value without given backend, in which case we keep our backend
          // and create the accessors
          Value<ValueType>& operator=(const Value<ValueType> &rightHandSide) {
            if(rightHandSide.hasActualValue) {
              hasActualValue = true;
              value = rightHandSide.value;
            }
            else {
              // we obtain the register accessor later, in case the map file was not yet parsed up to its definition
              hasActualValue = false;
              registerName = rightHandSide.registerName;
              accessor.reset();
            }
            return *this;
          }

        protected:

          // constructor: assume having an actual value by default
          Value(const boost::shared_ptr<DeviceBackend> &_backend)
          : hasActualValue(true), backend(_backend)
          {}

          // flag if the actual value is already known and thus the member variable "value" is valid.
          bool hasActualValue;

          // field to store the actual value
          ValueType value;

          // name of the register to obtain the value from
          std::string registerName;

          // backend to obtain the register accessors from
          boost::shared_ptr<DeviceBackend> backend;

          // register accessor to obtain the value, if not yet known upon construction
          mutable boost::shared_ptr< BufferingRegisterAccessorImpl<ValueType> > accessor;

          friend class LogicalNameMap;
          friend class LogicalNameMap::RegisterInfo;
      };

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

        protected:

          // constuctor: initialise Values
          RegisterInfo(const boost::shared_ptr<DeviceBackend> &backend)
          : targetType(TargetType::INVALID),
            deviceName(backend),
            registerName(backend),
            firstIndex(backend),
            length(backend),
            channel(backend),
            value(backend)
          {}

          friend class LogicalNameMap;
      };

      /** Class to store a register path */
      class RegisterPath {
        public:
          RegisterPath() : path(separator) {}
          RegisterPath(const std::string &_path) : path(separator+_path) {removeExtraSeparators();}
          RegisterPath(const RegisterPath &_path) : path(_path.path) {removeExtraSeparators();}

          /** type conversion operators into std::string */
          operator std::string&() { return path; }
          operator std::string() const { return path; }

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
      LogicalNameMap(const std::string &fileName, const boost::shared_ptr<DeviceBackend> &backend) {
        parseFile(fileName, backend);
      }

      /** Constructor: parse map from XML file. No backend is specified, so internal references cannot be resolved. */
      LogicalNameMap(const std::string &fileName) {
        parseFile(fileName, boost::shared_ptr<DeviceBackend>());
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
      void parseFile(const std::string &fileName, const boost::shared_ptr<DeviceBackend> &backend);

      /** called inside parseFile() to parse an XML element and its sub-elements recursivly */
      void parseElement(RegisterPath currentPath, const xmlpp::Element *element, const boost::shared_ptr<DeviceBackend> &backend);

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
