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
#include "VariableNetworkNode.h"

namespace ChimeraTK {

  class AccessorBase;

  /** This class describes a network of variables all connected to each other. */
  class VariableNetwork {

      VariableNetwork( const VariableNetwork& other ) = delete;         // non construction-copyable
      VariableNetwork& operator=( const VariableNetwork& ) = delete;    // non copyable

    public:

      VariableNetwork() {}

      /** Define trigger types. The trigger decides when values are fed into the network and distributed to the consumers. */
      enum class TriggerType {
          feeder,           ///< The feeder has an UpdateMode::push and thus decides when new values are fed
          pollingConsumer,  ///< If there is exacly one consumer with UpdateMode::poll, it will trigger the feeding
          external,         ///< another variable network can trigger the feeding of this network
          none              ///< no trigger has yet been selected
      };

      /** Add an node to the network. */
      void addNode(VariableNetworkNode &a);

      /** Add a trigger receiver node */
      void addTriggerReceiver(VariableNetwork *network);

      /** Check if the network already has a feeding node connected to it. */
      bool hasFeedingNode() const;

      /** Count the number of consuming nodes in the network */
      size_t countConsumingNodes() const;

      /** Obtain the type info of the UserType. If the network type has not yet been determined (i.e. if no output
       *  accessor has been assigned yet), the typeid of void will be returned. */
      const std::type_info& getValueType() const {
        return *valueType;
      }

      /** Return the feeding node */
      VariableNetworkNode getFeedingNode() const;

      /** Return list of consuming nodes */
      std::list<VariableNetworkNode> getConsumingNodes() const;

      /** Dump the network structure to std::cout. The optional linePrefix will be prepended to all lines. */
      void dump(const std::string& linePrefix="") const;

      /** Compare two networks */
      bool operator==(const VariableNetwork &other) const {
        if(other.valueType != valueType) return false;
        if(other.nodeList != nodeList) return false;
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

      /** Add an accessor belonging to another node as an external trigger to this network. Whenever the
       *  VariableNetwork of the given node will be fed with a new value, feeding of this network will be
       *  triggered as well. */
      void addTrigger(VariableNetworkNode trigger);

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

      /** List of nodes in the network */
      std::list<VariableNetworkNode> nodeList;

      /** The network value type id. Since in C++, std::type_info is non-copyable and typeid() returns a reference to
       *  an object with static storage duration, we have to (and can safely) store a pointer here. */
      const std::type_info* valueType{&typeid(AnyType)};

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
