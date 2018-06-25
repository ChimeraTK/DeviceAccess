#include "VisitorHelper.h"
#include "VariableNetworkNode.h"

namespace ChimeraTK {

namespace detail {

std::string encodeDotNodeName(std::string name) {
    std::replace(name.begin(), name.end(), ':', 'c'); // colon
    std::replace(name.begin(), name.end(), '/', 's'); // slash
    std::replace(name.begin(), name.end(), '.', 'd'); // dot
    std::replace(name.begin(), name.end(), ' ', '_'); // Generic space replacer
    std::replace(name.begin(), name.end(), '*', 'a'); // asterisk

    return name;
}

std::string nodeName(const VariableNetworkNode& node) {
    return node.getQualifiedName().empty() ? node.getName() : node.getQualifiedName();
}

} // namespace detail
} // namespace ChimeraTK
