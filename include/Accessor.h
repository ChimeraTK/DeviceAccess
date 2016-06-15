/*
 * Accessor.h
 *
 *  Created on: Jun 09, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_ACCESSOR_H
#define CHIMERATK_ACCESSOR_H

#include <string>

#include <ControlSystemAdapter/ProcessArray.h>
#include <mtca4u/RegisterPath.h>

#include "Application.h"
#include "ApplicationModule.h"

namespace ChimeraTK {

  using namespace mtca4u;

  /*********************************************************************************************************************/

  // stupid temporary base class for accessors, which is not templated @todo TODO replace with proper class structure
  class AccessorBase {
    public:
      virtual ~AccessorBase() {}

      /** Check if this accessor is a output or input variable, from the point-of-view of the ApplicationModule
       *  owning the instance of the Accessor */
      virtual bool isOutput() = 0;

      /** Return if the accessor is properly initialised. It is initialised if it was constructed passing the pointer
       *  to an implementation (a NDRegisterAccessor), it is not initialised if it was constructed only using the
       *  placeholder constructor without arguments. */
      virtual bool isInitialised() const = 0;

      /** Use a ProcessVariable as implementation. */
      virtual void useProcessVariable(boost::shared_ptr<ProcessVariable> &var) = 0;

      /* Obtain the type info of the UserType */
      virtual const std::type_info& getValueType() const = 0;

      /** Obtain direction of the accessor */
      virtual VariableDirection getDirection() const = 0;
  };

  /*********************************************************************************************************************/

  template< typename UserType >
  class Accessor : public AccessorBase {

    public:

      /** The default accessor takes no arguments and leaves the accessor uninitialised. It will be dysfunctional
       *  until it is properly initialised using connectTo(). */
      Accessor(ApplicationModule *owner, const std::string &name, VariableDirection direction, std::string unit,
          UpdateMode mode)
      : _owner(owner), _name(name), _direction(direction), _unit(unit), _mode(mode)
      {};

      /** Connect the accessor to another accessor */
      template< typename UserType_o >
      void connectTo(Accessor<UserType_o> &targetAccessor);

      /** Publish the variable to the control system under the given name */
      void publish(const std::string& name);

      /** Connect variable to a device register */
      void connectToDevice(const std::string &deviceAlias, const std::string &registerName,
          UpdateMode mode, size_t numberOfElements=1, size_t elementOffsetInRegister=0);

      virtual bool isOutput();

      VariableDirection getDirection() const {return _direction;}

      /* Obtain the unit of the variable */
      const std::string& getUnit() const {return _unit;}

      const std::type_info& getValueType() const {
        return typeid(UserType);
      }

    protected:

      ApplicationModule *_owner;
      std::string _name;
      VariableDirection _direction;
      std::string _unit;
      UpdateMode _mode;

  };

  /*********************************************************************************************************************/

  /** Connect the accessor to another accessor */
  template< typename UserType >
  template< typename UserType_o >
  void Accessor<UserType>::connectTo(Accessor<UserType_o> &targetAccessor) {
    assert( _direction != targetAccessor._direction );
    Application::getInstance().connectAccessors(*this, targetAccessor);
  }

  /*********************************************************************************************************************/

  template< typename UserType >
  void Accessor<UserType>::publish(const std::string& name) {
    Application::getInstance().publishAccessor(*this, name);
  }

  /*********************************************************************************************************************/

  template< typename UserType >
  bool Accessor<UserType>::isOutput() {
    return _direction == VariableDirection::output;
  };

  /*********************************************************************************************************************/

  template< typename UserType >
  void Accessor<UserType>::connectToDevice(const std::string &deviceAlias, const std::string &registerName,
      UpdateMode mode, size_t numberOfElements, size_t elementOffsetInRegister) {
    Application::getInstance().connectAccessorToDevice(*this, deviceAlias, registerName, mode, numberOfElements,
        elementOffsetInRegister);
  }

} /* namespace ChimeraTK */

#endif /* CHIMERATK_ACCESSOR_H */
