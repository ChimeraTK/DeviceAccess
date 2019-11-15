/*
 * Flags.h
 *
 *  Created on: Jun 14, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_FLAGS_H
#define CHIMERATK_FLAGS_H

namespace ChimeraTK {

  /** Struct to define the direction of variables. The main direction is defined
   * with an enum. In addition the presence of a return channel is specified. */
  struct VariableDirection {
    /** Enum to define directions of variables. The direction is always defined
     * from the point-of-view of the
     *  owner, i.e. the application module owning the instance of the accessor in
     * this context. */
    enum { consuming, feeding, invalid } dir;

    /** Presence of return channel */
    bool withReturn;

    /** Comparison */
    bool operator==(const VariableDirection& other) const { return dir == other.dir && withReturn == other.withReturn; }
    bool operator!=(const VariableDirection& other) const { return !operator==(other); }
  };

  /** Enum to define the update mode of variables. */
  enum class UpdateMode { poll, push, invalid };

  /** Enum to define types of VariableNetworkNode */
  enum class NodeType { Device, ControlSystem, Application, TriggerReceiver, TriggerProvider, Constant, invalid };

  /** Hierarchy modifier: specify if and how the module hierarchy should be modified in EntityOwner::findTag() etc. */
  enum class HierarchyModifier {
    none,       ///< No modification is performed
    hideThis,   ///< The hierarchy level at which this flag is specified is hidden. Everything below this level is moved
                ///< exactly one level up. The structure below this level is kept.
    moveToRoot, ///< The module at which this flag is specified is moved to the root level, together with the entire
                ///< structure below the module. Note: Unless you run findTag() or so on the entire application, the
                ///< moved hierarchy structures might not be visible in the control system etc.
    oneLevelUp, ///< Move the module up to the level where the owner lives. Instead of creating a "daughter"
                ///< of the owning module, it creates a "sister" (module that lives on the same level).
                ///< This modifyer can only be used in sub-modules, not on the first level
    oneUpAndHide///< Move the structure inside the module up to the level where the owner lives. Instead of adding a hierrarchy
    ///< level, one level is removed. This modifyer can only be used in sub-modules, not on the first level
    ///< inside an application.
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_FLAGS_H */
