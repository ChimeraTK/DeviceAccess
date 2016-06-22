/*
 * VariableNetwork.h
 *
 *  Created on: Jun 14, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_VARIABLE_NETWORK_H
#define CHIMERATK_VARIABLE_NETWORK_H

#include <list>
#include <string>
#include <iostream>
#include <typeinfo>
#include <boost/mpl/for_each.hpp>

#include <ControlSystemAdapter/ProcessVariable.h>

#include "Flags.h"

namespace xmlpp {
  class Element;
}

namespace ChimeraTK {

  class AccessorBase;

  /** This class describes a network of variables all connected to each other. */
  class VariableNetwork {

      VariableNetwork( const VariableNetwork& other ) = delete;         // non construction-copyable
      VariableNetwork& operator=( const VariableNetwork& ) = delete;    // non copyable

    public:

      VariableNetwork() {}

      /** Define accessor types */
      enum class NodeType {
          Device, ControlSystem, Application, TriggerReceiver, invalid
      };

      /** Define trigger types. The trigger decides when values are fed into the network and distributed to the consumers. */
      enum class TriggerType {
          feeder,           ///< The feeder has an UpdateMode::push and thus decides when new values are fed
          pollingConsumer,  ///< If there is exacly one consumer with UpdateMode::poll, it will trigger the feeding
          external,         ///< another variable network can trigger the feeding of this network
          none              ///< no trigger has yet been selected
      };

      /** Structure describing a node of the network */
      struct Node {
          NodeType type{NodeType::invalid};
          UpdateMode mode{UpdateMode::invalid};

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

          /** Function checking if the node requires a fixed implementation */
          bool hasImplementation() const;

          /** Compare two nodes */
          bool operator==(const Node& other) const;
          bool operator!=(const Node& other) const;

          /** Print node information to std::cout */
          void dump() const;

          /** Create an XML node describing this network node as seen by the control syste. If the type is not
           *  NodeType::ControlSystem, this function does nothing. Otherwise the correct directory hierarchy will be
           *  created (if not yet existing) and a variable tag will be created containing the externally visible
           *  properties of this variable. */
          void createXML(xmlpp::Element *rootElement) const;
      };

      /** Add an application-side node (i.e. an Accessor) to the network. */
      void addAppNode(AccessorBase &a);

      /** Add control-system-to-device publication. The given accessor will be used to derive the requred value type.
       *  The name will be the name of the process variable visible in the control system adapter. */
      void addFeedingPublication(AccessorBase &a, const std::string& name);

      /** Add control-system-to-device publication. The given accessor will be used to derive the requred value type.
       *  The name will be the name of the process variable visible in the control system adapter. */
      void addFeedingPublication(const std::type_info &typeInfo, const std::string& unit, const std::string& name);

      /** Add device-to-control-system publication. */
      void addConsumingPublication(const std::string& name);

      /** Add a device register as a consuming node (i.e. which will be written by this network) */
      void addConsumingDeviceRegister(const std::string &deviceAlias, const std::string &registerName);

      /** Add a device register as a feeding node (i.e. which will be read from this network) */
      void addFeedingDeviceRegister(AccessorBase &a, const std::string &deviceAlias, const std::string &registerName,
          UpdateMode mode);

      /** Add a device register as a feeding node (i.e. which will be read from this network) */
      void addFeedingDeviceRegister(const std::type_info &typeInfo, const std::string& unit,
          const std::string &deviceAlias, const std::string &registerName, UpdateMode mode);

      /** Add a trigger receiver node */
      void addTriggerReceiver(VariableNetwork *network);

      /** Check if the network already has a feeding node connected to it. */
      bool hasFeedingNode() const;

      /** Count the number of consuming nodes in the network */
      size_t countConsumingNodes() const;

      /** Count the number of nodes requiring a fixed implementation */
      size_t countFixedImplementations() const;

      /** Check if either of the given accessors is part of this network. If the second argument is omitted, only
       *  the first accessor will be checked. */
      bool hasAppNode(AccessorBase *a, AccessorBase *b=nullptr) const;

      /** Obtain the type info of the UserType. If the network type has not yet been determined (i.e. if no output
       *  accessor has been assigned yet), the typeid of void will be returned. */
      const std::type_info& getValueType() const {
        return *valueType;
      }

      /** Return the feeding node */
      const Node& getFeedingNode() const { return feeder; }

      /** Return list of consuming nodes */
      const std::list<Node>& getConsumingNodes() const { return consumerList; }

      /** Dump the network structure to std::cout. The optional linePrefix will be prepended to all lines. */
      void dump(const std::string& linePrefix="") const;

      /** Compare two networks */
      bool operator==(const VariableNetwork &other) const {
        if(other.feeder != feeder) return false;
        if(other.valueType != valueType) return false;
        if(other.consumerList != consumerList) return false;
        return true;
      }
      bool operator!=(const VariableNetwork &other) const {
        return !operator==(other);
      }

      /** Return the trigger type. This function will also do some checking if the network confguration is valid under
       *  the aspect of the trigger type. */
      TriggerType getTriggerType() const;

      /** Return the enginerring unit */
      const std::string& getUnit() { return engineeringUnit; }

      /** Return the network providing the external trigger to this network, if TriggerType::external. If the network
       *  has another trigger type, an exception will be thrown. */
      VariableNetwork& getExternalTrigger();

      /** Add an accessor belonging to another network as an external trigger to this network. Whenever the
       *  VariableNetwork of the given accessor will be fed with a new value, feeding of this network will be
       *  triggered as well. */
      void addTrigger(AccessorBase &trigger);

      /** Check if the network is legally configured */
      void check();

      /** Check the flag if the network connections has been created already */
      bool isCreated() const { return flagIsCreated; }

      /** Set the flag that the network connections are created */
      void markCreated() { flagIsCreated = true; }

      /** Assign a ProcessVariable as implementation for the external trigger */
      void setExternalTriggerImpl(boost::shared_ptr< mtca4u::ProcessVariable > impl) {
        externalTriggerImpl = impl;
      }

      /** */
      boost::shared_ptr< mtca4u::ProcessVariable > getExternalTriggerImpl() const {
        return externalTriggerImpl;
      }

    protected:

      /** Feeding node (i.e. the node providing values to the network) */
      Node feeder;

      /** List of consuming nodes (i.e. the nodes receiving values from the network) */
      std::list<Node> consumerList;

      /** The network value type id. Since in C++, std::type_info is non-copyable and typeid() returns a reference to
       *  an object with static storage duration, we have to (and can safely) store a pointer here. */
      const std::type_info* valueType{&typeid(void)};

      /** Engineering unit */
      std::string engineeringUnit;

      /** Flag if an external trigger has been added to this network */
      bool hasExternalTrigger{false};

      /** Pointer to the network providing the external trigger */
      VariableNetwork *externalTrigger{nullptr};

      /** Pointer to ProcessVariable providing the trigger (if external trigger is enabled) */
      boost::shared_ptr< mtca4u::ProcessVariable > externalTriggerImpl;

      /** Flag if the network connections have been created already */
      bool flagIsCreated{false};

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_VARIABLE_NETWORK_H */
