/*
 * VariableNetworkNode.h
 *
 *  Created on: Jun 23, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_VARIABLE_NETWORK_NODE_H
#define CHIMERATK_VARIABLE_NETWORK_NODE_H

#include "Flags.h"

namespace xmlpp {
  class Element;
}

namespace ChimeraTK {

  class VariableNetwork;
  class AccessorBase;

  /** Class describing a node of a variable network */
  class VariableNetworkNode {
    public:
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
      bool operator==(const VariableNetworkNode& other) const;
      bool operator!=(const VariableNetworkNode& other) const;

      /** Print node information to std::cout */
      void dump() const;

      /** Create an XML node describing this network node as seen by the control syste. If the type is not
       *  NodeType::ControlSystem, this function does nothing. Otherwise the correct directory hierarchy will be
       *  created (if not yet existing) and a variable tag will be created containing the externally visible
       *  properties of this variable. */
      void createXML(xmlpp::Element *rootElement) const;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_VARIABLE_NETWORK_NODE_H */
