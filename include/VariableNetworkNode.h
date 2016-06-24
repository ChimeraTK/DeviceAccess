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

#include "Flags.h"

namespace xmlpp {
  class Element;
}

namespace ChimeraTK {

  class VariableNetwork;
  class AccessorBase;

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
      VariableNetworkNode(AccessorBase &accessor);

      /** Constructor for a Device node */
      VariableNetworkNode(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode,
          VariableDirection direction, const std::type_info &valTyp=typeid(AnyType));

      /** Constructor for a ControlSystem node */
      VariableNetworkNode(std::string publicName, VariableDirection direction,
          const std::type_info &valTyp=typeid(AnyType));

      /** Constructor for a TriggerReceiver node triggering the data transfer of another network */
      VariableNetworkNode(VariableNetwork *networkToTrigger);

      /** Default constructor for an invalid node */
      VariableNetworkNode() : pdata(new data) {}

      /** Set the owner network of this node. If an owner network is already set, an assertion will be raised */
      void setOwner(VariableNetwork *network);

      /** Set the value type for this node. Only possible of the current value type is undecided (i.e. AnyType). */
      void setValueType(const std::type_info& newType) const {
        assert(*pdata->valueType == typeid(AnyType));
        pdata->valueType = &newType;
      }

      /** Function checking if the node requires a fixed implementation */
      bool hasImplementation() const;

      /** Compare two nodes */
      bool operator==(const VariableNetworkNode& other) const;
      bool operator!=(const VariableNetworkNode& other) const;

      /** Connect two nodes */
      VariableNetworkNode& operator<<(const VariableNetworkNode &other);
      VariableNetworkNode& operator>>(const VariableNetworkNode &other);

      /** Add a trigger */
      VariableNetworkNode& operator[](const VariableNetworkNode &trigger);

      /** Print node information to std::cout */
      void dump() const;

      /** Create an XML node describing this network node as seen by the control syste. If the type is not
       *  NodeType::ControlSystem, this function does nothing. Otherwise the correct directory hierarchy will be
       *  created (if not yet existing) and a variable tag will be created containing the externally visible
       *  properties of this variable. */
      void createXML(xmlpp::Element *rootElement) const;

      /** Check if the node already has an owner */
      bool hasOwner() const { return pdata->network != nullptr; }

      /** Getter for the properties */
      NodeType getType() const { return pdata->type; }
      UpdateMode getMode() const { return pdata->mode; }
      VariableDirection getDirection() const { return pdata->direction; }
      const std::type_info& getValueType() const { return *(pdata->valueType); }
      const std::string& getUnit() const { return pdata->unit; }
      VariableNetwork& getOwner() const { assert(pdata->network != nullptr); return *(pdata->network); }
      AccessorBase& getAppAccessor() const { assert(pdata->appNode != nullptr); return *(pdata->appNode); }
      VariableNetwork& getTriggerReceiver() const { assert(pdata->triggerReceiver != nullptr); return *(pdata->triggerReceiver); }
      const std::string& getPublicName() const { assert(pdata->type == NodeType::ControlSystem); return pdata->publicName; }
      const std::string& getDeviceAlias() const { assert(pdata->type == NodeType::Device); return pdata->deviceAlias; }
      const std::string& getRegisterName() const { assert(pdata->type == NodeType::Device); return pdata->registerName; }

    protected:

      struct data {

        data() {}

        /** prevent copies of the data container */
        data(data const &) = delete;
        data(data const &&) = delete;
        data& operator=(data const &) = delete;

        /** Type of the node (Application, Device, ControlSystem, Trigger) */
        NodeType type{NodeType::invalid};

        /** Update mode: poll or push */
        UpdateMode mode{UpdateMode::invalid};

        /** Node direction: feeding or consuming */
        VariableDirection direction{VariableDirection::invalid};

        /** Value type of this node. If the type_info is the typeid of AnyType, the actual type can be decided when making
         *  the connections. */
        const std::type_info* valueType{&typeid(AnyType)};

        /** Engineering unit. If "arbitrary", no unit has been defined (and any unit is allowed). */
        std::string unit{"arbitrary"};

        /** The network this node belongs to */
        VariableNetwork *network{nullptr};

        /** Pointer to Accessor if type == Application */
        AccessorBase *appNode{nullptr};

        /** Pointer to network which should be triggered by this node */
        VariableNetwork *triggerReceiver{nullptr};

        /** Public name if type == ControlSystem */
        std::string publicName;

        /** Device information if type == Device */
        std::string deviceAlias;
        std::string registerName;

      };

      boost::shared_ptr<data> pdata;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_VARIABLE_NETWORK_NODE_H */
