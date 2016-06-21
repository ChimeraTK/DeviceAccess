/*
 * Accessor.h
 *
 *  Created on: Jun 09, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_ACCESSOR_H
#define CHIMERATK_ACCESSOR_H

#include <string>

#include <mtca4u/RegisterPath.h>

#include "Application.h"
#include "ApplicationModule.h"

namespace ChimeraTK {

  using namespace mtca4u;

  class Application;

  /*********************************************************************************************************************/

  // stupid temporary base class for accessors, which is not templated @todo TODO replace with proper class structure
  class AccessorBase {
    public:
      virtual ~AccessorBase() {}

      /** Check if this accessor is a output or input variable, from the point-of-view of the ApplicationModule
       *  owning the instance of the Accessor. A feeding accessor feeds values to its peers and thus is an output
       *  accessor. */
      virtual bool isFeeding() = 0;

      /** Return if the accessor is properly initialised. It is initialised if it was constructed passing the pointer
       *  to an implementation (a NDRegisterAccessor), it is not initialised if it was constructed only using the
       *  placeholder constructor without arguments. */
      virtual bool isInitialised() const = 0;

      /** Use a ProcessVariable as implementation. */
      virtual void useProcessVariable(const boost::shared_ptr<ProcessVariable> &var) = 0;

      /* Obtain the type info of the UserType */
      virtual const std::type_info& getValueType() const = 0;

      /** Obtain direction of the accessor */
      virtual VariableDirection getDirection() const = 0;

      /** Obtain the update mode of the accessor */
      virtual UpdateMode getUpdateMode() const = 0;

      /* Obtain the unit of the variable */
      virtual const std::string& getUnit() const = 0;
};

  /*********************************************************************************************************************/

  /** An invalid instance which can be used e.g. for optional arguments passed by reference */
  class InvalidAccessor : public AccessorBase {
    public:
      constexpr InvalidAccessor() {}
      ~InvalidAccessor() {}
      bool isFeeding() {std::terminate();}
      bool isInitialised() const {std::terminate();}
      void useProcessVariable(const boost::shared_ptr<ProcessVariable> &) {std::terminate();}
      const std::type_info& getValueType() const {std::terminate();}
      VariableDirection getDirection() const {std::terminate();}
      UpdateMode getUpdateMode() const {std::terminate();}
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

      /** Publish the variable to the control system as a control-system-to-application variable under the given name */
      void consumeFromControlSystem(const std::string& name);

      /** Publish the variable to the control system as a application-to-control-system variable under the given name */
      void feedToControlSystem(const std::string& name);

      /** Connect variable to a device register and request that the variable will "consume" data from the register. */
      void consumeFromDevice(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode);

      /** Connect variable to a device register and request that the variable will "feed" data to the register.*/
      void feedToDevice(const std::string &deviceAlias, const std::string &registerName);

      /** Add another accessor as an external trigger */
      void addTrigger(AccessorBase &trigger);

      virtual bool isFeeding();

      VariableDirection getDirection() const {return _direction;}

      UpdateMode getUpdateMode() const {return _mode;}

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
  void Accessor<UserType>::feedToControlSystem(const std::string& name) {
    assert( _direction == VariableDirection::feeding );
    VariableNetwork &network = Application::getInstance().findOrCreateNetwork(this);
    network.addAppNode(*this);
    network.addConsumingPublication(name);
  }

  /*********************************************************************************************************************/

  template< typename UserType >
  void Accessor<UserType>::consumeFromControlSystem(const std::string& name) {
    assert( _direction == VariableDirection::consuming );
    VariableNetwork &network = Application::getInstance().findOrCreateNetwork(this);
    network.addAppNode(*this);
    network.addFeedingPublication(*this,name);
  }

  /*********************************************************************************************************************/

  template< typename UserType >
  bool Accessor<UserType>::isFeeding() {
    return _direction == VariableDirection::feeding;
  };

  /*********************************************************************************************************************/

  template< typename UserType >
  void Accessor<UserType>::consumeFromDevice(const std::string &deviceAlias, const std::string &registerName,
      UpdateMode mode) {
    assert( _direction == VariableDirection::consuming );
    VariableNetwork &network = Application::getInstance().findOrCreateNetwork(this);
    network.addAppNode(*this);
    network.addFeedingDeviceRegister(*this, deviceAlias, registerName, mode);
  }

  /*********************************************************************************************************************/

  template< typename UserType >
  void Accessor<UserType>::feedToDevice(const std::string &deviceAlias, const std::string &registerName) {
    assert( _direction == VariableDirection::feeding );
    VariableNetwork &network = Application::getInstance().findOrCreateNetwork(this);
    network.addAppNode(*this);
    network.addConsumingDeviceRegister(deviceAlias, registerName);
  }

  /*********************************************************************************************************************/

  template< typename UserType >
  void Accessor<UserType>::addTrigger(AccessorBase &trigger) {
    VariableNetwork &network = Application::getInstance().findOrCreateNetwork(this);
    network.addAppNode(*this);
    network.addTrigger(trigger);
  }

} /* namespace ChimeraTK */

#endif /* CHIMERATK_ACCESSOR_H */
