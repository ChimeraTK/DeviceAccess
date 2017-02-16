/*
 * VariableNetworkNode.h
 *
 *  Created on: Jun 23, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_VARIABLE_NETWORK_NODE_H
#define CHIMERATK_VARIABLE_NETWORK_NODE_H

#include <assert.h>

#include <boost/shared_ptr.hpp>

#include <mtca4u/NDRegisterAccessorBridge.h>

#include "Flags.h"
#include "ConstantAccessor.h"

namespace xmlpp {
  class Element;
}

namespace ChimeraTK {

  class VariableNetwork;
  class AccessorBase;
  struct VariableNetworkNode_data;

  /** Pseudo type to identify nodes which can have arbitrary types */
  class AnyType {};

  /** Class describing a node of a variable network */
  class VariableNetworkNode {

    public:

      /** Copy-constructor: Just copy the pointer to the data storage object */
      VariableNetworkNode(const VariableNetworkNode &other);

      /** Copy by assignment operator: Just copy the pointer to the data storage object */
      VariableNetworkNode& operator=(const VariableNetworkNode &rightHandSide);

      /** Constructor for an Application node */
      VariableNetworkNode(mtca4u::TransferElement *accessorBridge, const std::string &name,
          VariableDirection direction, std::string unit, size_t nElements, UpdateMode mode,
          const std::string &description, const std::type_info* valueType);

      /** Constructor for a Device node */
      VariableNetworkNode(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode,
          VariableDirection direction, const std::type_info &valTyp=typeid(AnyType), size_t nElements=0);

      /** Constructor for a ControlSystem node */
      VariableNetworkNode(std::string publicName, VariableDirection direction,
          const std::type_info &valTyp=typeid(AnyType), size_t nElements=0);

      /** Constructor for a TriggerReceiver node triggering the data transfer of another network. The additional dummy
       *  argument is only there to discriminate the signature from the copy constructor and will be ignored. */
      VariableNetworkNode(VariableNetworkNode& nodeToTrigger, int);

      /** Default constructor for an invalid node */
      VariableNetworkNode();
      
      /** Factory function for a constant (a constructor cannot be templated) */
      template<typename UserType>
      static VariableNetworkNode makeConstant(bool makeFeeder, UserType value=0, size_t length=1);

      /** Set the owner network of this node. If an owner network is already set, an assertion will be raised */
      void setOwner(VariableNetwork *network);

      /** Set the value type for this node. Only possible of the current value type is undecided (i.e. AnyType). */
      void setValueType(const std::type_info& newType) const;

      /** Function checking if the node requires a fixed implementation */
      bool hasImplementation() const;

      /** Compare two nodes */
      bool operator==(const VariableNetworkNode& other) const;
      bool operator!=(const VariableNetworkNode& other) const;

      /** Connect two nodes */
//      VariableNetworkNode& operator<<(const VariableNetworkNode &other);
      VariableNetworkNode operator>>(VariableNetworkNode other);

      /** Add a trigger */
      VariableNetworkNode operator[](VariableNetworkNode trigger);

      /** Check for presence of an external trigger */
      bool hasExternalTrigger() const;

      /** Return the external trigger node. if no external trigger is present, an assertion will be raised. */
      VariableNetworkNode getExternalTrigger();

      /** Print node information to std::cout */
      void dump() const;

      /** Create an XML node describing this network node as seen by the control syste. If the type is not
       *  NodeType::ControlSystem, this function does nothing. Otherwise the correct directory hierarchy will be
       *  created (if not yet existing) and a variable tag will be created containing the externally visible
       *  properties of this variable. */
      void createXML(xmlpp::Element *rootElement) const;

      /** Check if the node already has an owner */
      bool hasOwner() const;

      /** Getter for the properties */
      NodeType getType() const;
      UpdateMode getMode() const;
      VariableDirection getDirection() const;
      const std::type_info& getValueType() const;
      const std::string& getName() const;
      const std::string& getUnit() const;
      const std::string& getDescription() const;
      VariableNetwork& getOwner() const;
      VariableNetworkNode getTriggerReceiver();
      const std::string& getPublicName() const;
      const std::string& getDeviceAlias() const;
      const std::string& getRegisterName() const;
      void setNumberOfElements(size_t nElements);
      size_t getNumberOfElements() const;
      mtca4u::TransferElement& getAppAccessorNoType();

      template<typename UserType>
      mtca4u::NDRegisterAccessorBridge<UserType>& getAppAccessor() const;


      template<typename UserType>
      boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> getConstAccessor() const;
      
      /** Return the unique ID of this node (will change every time the application is started). */
      const void* getUniqueId() const {return pdata.get();}

    protected:

      boost::shared_ptr<VariableNetworkNode_data> pdata;
  };

  /*********************************************************************************************************************/

  /** We use a pimpl pattern so copied instances of VariableNetworkNode refer to the same instance of the data
    *  structure and thus stay consistent all the time. */
  struct VariableNetworkNode_data {

    VariableNetworkNode_data() {}

    /** Type of the node (Application, Device, ControlSystem, Trigger) */
    NodeType type{NodeType::invalid};

    /** Update mode: poll or push */
    UpdateMode mode{UpdateMode::invalid};

    /** Node direction: feeding or consuming */
    VariableDirection direction{VariableDirection::invalid};

    /** Value type of this node. If the type_info is the typeid of AnyType, the actual type can be decided when making
      *  the connections. */
    const std::type_info* valueType{&typeid(AnyType)};

    /** Engineering unit. If equal to mtca4u::TransferElement::unitNotSet, no unit has been defined (and any unit is allowed). */
    std::string unit{mtca4u::TransferElement::unitNotSet};

    /** Description */
    std::string description{""};

    /** The network this node belongs to */
    VariableNetwork *network{nullptr};

    /** Pointer to implementation if type == Constant */
    boost::shared_ptr<mtca4u::TransferElement> constNode;

    /** Pointer to implementation if type == Application */
    mtca4u::TransferElement *appNode{nullptr};

    /** Pointer to network which should be triggered by this node */
    VariableNetworkNode triggerReceiver;

    /** Pointer to the network providing the external trigger. May only be used for feeding nodes with an
      *  update mode poll. When enabled, the update mode will be converted into push. */
    VariableNetworkNode externalTrigger;

    /** Public name if type == ControlSystem */
    std::string publicName;

    /** Accessor name if type == Application */
    std::string name;

    /** Device information if type == Device */
    std::string deviceAlias;
    std::string registerName;
    
    /** Number of elements in the variable. 0 means not yet decided. */
    size_t nElements{0};

  };

  /*********************************************************************************************************************/
  /*** Implementations *************************************************************************************************/
  /*********************************************************************************************************************/

  template<typename UserType>
  VariableNetworkNode VariableNetworkNode::makeConstant(bool makeFeeder, UserType value, size_t length) {
    VariableNetworkNode node;
    node.pdata.reset(new VariableNetworkNode_data);
    node.pdata->constNode.reset(new ConstantAccessor<UserType>(value, length));
    node.pdata->type = NodeType::Constant;
    node.pdata->valueType = &typeid(UserType);
    node.pdata->nElements = length;
    if(makeFeeder) {
      node.pdata->direction = VariableDirection::feeding;
      node.pdata->mode = UpdateMode::push;
    }
    else {
      node.pdata->direction = VariableDirection::consuming;
      node.pdata->mode = UpdateMode::poll;
    }
    return node;
  }

  /*********************************************************************************************************************/
  
  template<typename UserType>
  mtca4u::NDRegisterAccessorBridge<UserType>& VariableNetworkNode::getAppAccessor() const {
    assert(typeid(UserType) == getValueType());
    auto accessor = dynamic_cast<mtca4u::NDRegisterAccessorBridge<UserType>*>(pdata->appNode);
    assert(accessor != nullptr);
    return *accessor;
  }

  /*********************************************************************************************************************/
  
  template<typename UserType>
  boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> VariableNetworkNode::getConstAccessor() const {
    return boost::dynamic_pointer_cast<mtca4u::NDRegisterAccessor<UserType>>(pdata->constNode);
  }

} /* namespace ChimeraTK */

#endif /* CHIMERATK_VARIABLE_NETWORK_NODE_H */
