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

#include "Flags.h"

namespace ChimeraTK {

  class AccessorBase;

  /** This class describes a network of variables all connected to each other. */
  class VariableNetwork {

    public:

      /** Define accessor types */
      enum class NodeType {
          Device, ControlSystem, Application, Undefined
      };

      /** Structure describing a node of the network */
      struct Node {
          NodeType type{NodeType::Undefined};
          UpdateMode mode{UpdateMode::invalid};

          /** Pointer to Accessor if type == Application */
          AccessorBase *appNode{nullptr};

          /** Public name if type == ControlSystem */
          std::string publicName;

          /** Device information if type == Device */
          std::string deviceAlias;
          std::string registerName;

          /** Function checking if the node requires a fixed implementation */
          bool hasImplementation() const {
            return type == NodeType::Device || type == NodeType::ControlSystem;
          }

          /** Print node information to std::cout */
          void dump() const {
            if(type == NodeType::Application) std::cout << " type = Application" << std::endl;
            if(type == NodeType::ControlSystem) std::cout << " type = ControlSystem ('" << publicName << "')" << std::endl;
            if(type == NodeType::Device) std::cout << " type = Device (" << deviceAlias << ": "
                << registerName << ")" << std::endl;
          }
      };

      /** Add an application-side node (i.e. an Accessor) to the network. */
      void addAppNode(AccessorBase &a);

      /** Add control-system-to-device publication. The given accessor will be used to derive the requred value type.
       *  The name will be the name of the process variable visible in the control system adapter. */
      void addFeedingPublication(AccessorBase &a, const std::string& name);

      /** Add device-to-control-system publication. */
      void addConsumingPublication(const std::string& name);

      /** Add a device register as a consuming node (i.e. which will be written by this network) */
      void addConsumingDeviceRegister(const std::string &deviceAlias, const std::string &registerName,
          UpdateMode mode, size_t numberOfElements, size_t elementOffsetInRegister);

      /** Add a device register as a feeding node (i.e. which will be read from this network) */
      void addFeedingDeviceRegister(AccessorBase &a, const std::string &deviceAlias, const std::string &registerName,
          UpdateMode mode, size_t numberOfElements, size_t elementOffsetInRegister);

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

      /** Dump the network structure to std::cout */
      void dump() const;

    protected:

      /** Feeding node (i.e. the node providing values to the network) */
      Node feeder;

      /** List of consuming nodes (i.e. the nodes receiving values from the network) */
      std::list<Node> consumerList;

      /** The network value type id. Since in C++, std::type_info is non-copyable and typeid() returns a reference to
       *  an object with static storage duration, we have to (and can safely) store a pointer here. */
      const std::type_info* valueType{&typeid(void)};

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_VARIABLE_NETWORK_H */
