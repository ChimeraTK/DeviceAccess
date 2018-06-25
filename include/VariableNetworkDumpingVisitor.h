#pragma once

#include "Visitor.h"
#include "VariableNetworkNodeDumpingVisitor.h"

#include <string>

namespace ChimeraTK {

// Forward declarations
class VariableNetwork;

/**
 * @brief The VariableNetworkDumpingVisitor class
 *
 * This class provides a textual dump of the VariableNetwork
 */
class VariableNetworkDumpingVisitor : public Visitor<VariableNetwork>, public VariableNetworkNodeDumpingVisitor {
public:
    VariableNetworkDumpingVisitor(const std::string &prefix, std::ostream &stream);
    virtual ~VariableNetworkDumpingVisitor() {}
    void dispatch(const VariableNetwork& t);
    using Visitor<VariableNetworkNode>::dispatch;

private:
    std::string _prefix;
};

} // namespace ChimeraTK
