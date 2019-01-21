/*
 * Flags.h
 *
 *  Created on: Jun 14, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_FLAGS_H
#define CHIMERATK_FLAGS_H

namespace ChimeraTK {

  /** Struct to define the direction of variables. The main direction is defined with an enum. In addition the presence
   *  of a return channel is specified. */
  struct VariableDirection {
      /** Enum to define directions of variables. The direction is always defined from the point-of-view of the
       *  owner, i.e. the application module owning the instance of the accessor in this context. */
      enum {
        consuming, feeding, invalid
      } dir;

      /** Presence of return channel */
      bool withReturn;

      /** Comparison */
      bool operator==(const VariableDirection &other) const {
        return dir == other.dir && withReturn == other.withReturn;
      }
      bool operator!=(const VariableDirection &other) const {
        return !operator==(other);
      }
  };

  /** Enum to define the update mode of variables. */
  enum class UpdateMode {
      poll, push, invalid
  };

  /** Enum to define types of VariableNetworkNode */
  enum class NodeType {
      Device, ControlSystem, Application, TriggerReceiver, TriggerProvider, Constant, invalid
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_FLAGS_H */
