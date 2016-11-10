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
          VariableDirection direction, const std::type_info &valTyp=typeid(AnyType), size_t nElements=0);

      /** Constructor for a ControlSystem node */
      VariableNetworkNode(std::string publicName, VariableDirection direction,
          const std::type_info &valTyp=typeid(AnyType), size_t nElements=0);

      /** Constructor for a TriggerReceiver node triggering the data transfer of another network. The additional dummy
       *  argument is only there to discriminate the signature from the copy constructor and will be ignored. */
      VariableNetworkNode(VariableNetworkNode& nodeToTrigger, int);

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
//      VariableNetworkNode& operator<<(const VariableNetworkNode &other);
      VariableNetworkNode operator>>(VariableNetworkNode other);

      /** Add a trigger */
      VariableNetworkNode operator[](VariableNetworkNode trigger);

      /** Check for presence of an external trigger */
      bool hasExternalTrigger() const { return pdata->externalTrigger != nullptr; }

      /** Return the external trigger node. if no external trigger is present, an assertion will be raised. */
      VariableNetworkNode& getExternalTrigger() const { assert(pdata->externalTrigger != nullptr); return *(pdata->externalTrigger); }

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
      const std::string& getDescription() const { return pdata->description; }
      VariableNetwork& getOwner() const { assert(pdata->network != nullptr); return *(pdata->network); }
      AccessorBase& getAppAccessor() const { assert(pdata->appNode != nullptr); return *(pdata->appNode); }
      VariableNetworkNode& getTriggerReceiver() const { assert(pdata->triggerReceiver != nullptr); return *(pdata->triggerReceiver); }
      const std::string& getPublicName() const { assert(pdata->type == NodeType::ControlSystem); return pdata->publicName; }
      const std::string& getDeviceAlias() const { assert(pdata->type == NodeType::Device); return pdata->deviceAlias; }
      const std::string& getRegisterName() const { assert(pdata->type == NodeType::Device); return pdata->registerName; }
      void setNumberOfElements(size_t nElements) { pdata->nElements = nElements; }
      size_t getNumberOfElements() const { return pdata->nElements; }

    protected:

      /** We use a pimpl pattern so copied instances of VariableNetworkNode refer to the same instance of the data
       *  structure and thus stay consistent all the time. */
      struct data {

        data() {}

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

        /** Description */
        std::string description{""};

        /** The network this node belongs to */
        VariableNetwork *network{nullptr};

        /** Pointer to Accessor if type == Application */
        AccessorBase *appNode{nullptr};

        /** Pointer to network which should be triggered by this node */
        VariableNetworkNode *triggerReceiver{nullptr};

        /** Pointer to the network providing the external trigger. May only be used for feeding nodes with an
         *  update mode poll. When enabled, the update mode will be converted into push. */
        VariableNetworkNode *externalTrigger{nullptr};

        /** Public name if type == ControlSystem */
        std::string publicName;

        /** Device information if type == Device */
        std::string deviceAlias;
        std::string registerName;
        
        /** Number of elements in the variable. 0 means not yet decided. */
        size_t nElements{0};

      };

      boost::shared_ptr<data> pdata;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_VARIABLE_NETWORK_NODE_H */
