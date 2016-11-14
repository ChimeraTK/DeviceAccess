/*
 * Flags.h
 *
 *  Created on: Jun 14, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_FLAGS_H
#define CHIMERATK_FLAGS_H

namespace ChimeraTK {

  /** Enum to define directions of variables. The direction is always defined from the point-of-view of the
   *  definer, i.e. the application module owning the instance of the accessor in this context. */
  enum class VariableDirection {
    consuming, feeding, invalid
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
