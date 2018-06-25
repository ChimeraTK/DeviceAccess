#pragma once

#include <string>
#include <ostream>

#include "Visitor.h"

namespace ChimeraTK {
// Forward declarations
class VariableNetworkNode;
class EntityOwner;
class Module;

/**
 * @brief The ModuleGraphVisitor class
 *
 * This class is responsible for generating the Graphiviz representation of the module hierarchy.
 */
class ModuleGraphVisitor : public Visitor<EntityOwner, Module, VariableNetworkNode> {
public:
    ModuleGraphVisitor(std::ostream& stream, bool showVariables = true);
    virtual ~ModuleGraphVisitor() {}

    void dispatch(const EntityOwner &owner);
    void dispatch(const Module &module);
    void dispatch(const VariableNetworkNode &node);
private:
    std::ostream& _stream;
    bool _showVariables;

    void dumpEntityOwner(const EntityOwner &owner);
};
} // namespace ChimeraTK
