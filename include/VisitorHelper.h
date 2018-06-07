#pragma once

#include <string>

namespace ChimeraTK {
class VariableNetworkNode;

namespace detail {

std::string encodeDotNodeName(std::string name);
std::string nodeName(const VariableNetworkNode& node);

} // namespace detail)
}// namespace ChimeraTK
