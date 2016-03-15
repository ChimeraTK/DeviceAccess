/*
 * DynamicValue.h
 *
 *  Created on: Feb 26, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_DYNAMIC_VALUE_H
#define MTCA4U_DYNAMIC_VALUE_H

#include <type_traits>
#include <string>

#include "DeviceBackend.h"

namespace mtca4u {

  // some forward declarations
  class DeviceBackend;

  template<typename UserType>
  class BufferingRegisterAccessorImpl;

  /** Hold a value of a RegisterInfo field (with proper resolution of dynamic references to other registers) */
  template<typename ValueType>
  class DynamicValue {

    public:

      /** obtain value via implicit type conversion operator */
      operator const ValueType&() const {
        if(!hasActualValue) {
          if(!accessor) {
            throw DeviceException("Cannot obtain this value before Value::createInternalAccessors() was called.",
                DeviceException::EX_NOT_OPENED);
          }
          accessor->read();
          return (*accessor)[0];
        }
        return value;
      }

      /** assignment operator with a value of the type ValueType. This will make the Value having an "actual"
       *  value, i.e. no dynamic references are present. */
      DynamicValue<ValueType>& operator=(const ValueType &rightHandSide) {
        value = rightHandSide;
        hasActualValue = true;
        return *this;
      }

      /** allows comparisons with char* if ValueType is std::string */
      bool operator==(const char *rightHandSide) {
        return value == rightHandSide;
      }

      /** constructor: assume having an actual value by default. If it does not have an actual value, the
       *  function createInternalAccessors() must be called before obtaining the value. */
      DynamicValue()
      : hasActualValue(true)
      {}

      /** assignment operator: allow assigment to Value without given backend, in which case we keep our backend
       *  and create the accessors. This version of the operator accepts any arithmetic-typed Value. */
      template < class rhsValueType,
                 class = typename std::enable_if<std::is_arithmetic<rhsValueType>::value>::type>
      DynamicValue<ValueType>& operator=(const DynamicValue<rhsValueType> &rightHandSide) {
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

      /** assignment operator: allow assigment to Value without given backend, in which case we keep our backend
       *  and create the accessors. This version of the operator accepts a string-typed Value. */
      DynamicValue<ValueType>& operator=(const DynamicValue<std::string> &rightHandSide);

      /** create the internal register accessor(s) to obtain the value, if needed */
      void createInternalAccessors(boost::shared_ptr<DeviceBackend> &backend) {
        if(!hasActualValue) {
          accessor = backend->getBufferingRegisterAccessor<ValueType>("",registerName);
        }
      }

      /** flag if the actual value is already known and thus the member variable "value" is valid. */
      bool hasActualValue;

      /** name of the register to obtain the value from */
      std::string registerName;

    protected:

      /** field to store the actual value */
      ValueType value;

      /** register accessor to obtain the value, if not yet known upon construction */
      mutable boost::shared_ptr< BufferingRegisterAccessorImpl<ValueType> > accessor;

      /** all Values are friends */
      template<typename T>
      friend class DynamicValue;
  };

  /********************************************************************************************************************/

  template<typename ValueType>
  DynamicValue<ValueType>& DynamicValue<ValueType>::operator=(const DynamicValue<std::string> &rightHandSide) {
    if(rightHandSide.hasActualValue) {
      hasActualValue = true;
      if(std::numeric_limits<ValueType>::is_integer) {
        value = std::stoi(rightHandSide.value);
      }
      else {
        value = std::stod(rightHandSide.value);
      }
    }
    else {
      // we obtain the register accessor later, in case the map file was not yet parsed up to its definition
      hasActualValue = false;
      registerName = rightHandSide.registerName;
      accessor.reset();
    }
    return *this;
  }

  /********************************************************************************************************************/

  template<>
  DynamicValue<std::string>& DynamicValue<std::string>::operator=(const DynamicValue<std::string> &rightHandSide);

} /* namespace mtca4u */

#endif /* MTCA4U_DYNAMIC_VALUE_H */
