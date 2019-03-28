#pragma once

#include "VariableNetworkNodeDumpingVisitor.h"
#include "Visitor.h"

#include <map>

namespace ChimeraTK {

  // Forward Declarations

  class Application;
  class VariableNetwork;

  /**
   * @brief The VariableNetworkGraphDumpingVisitor class
   *
   * This class provides a Graphiviz dump of the VariableNetwork.
   * Due to the potential size of the resulting graph, it is recommended to use
   * SVG for rendering the resulting graph.
   */
  class VariableNetworkGraphDumpingVisitor : public Visitor<Application, VariableNetwork>,
                                             VariableNetworkNodeDumpingVisitor {
   public:
    VariableNetworkGraphDumpingVisitor(std::ostream& stream);
    virtual ~VariableNetworkGraphDumpingVisitor() {}
    void dispatch(const Application& t);
    void dispatch(const VariableNetwork& t);
    void dispatch(const VariableNetworkNode& t);

   private:
    std::map<std::string, std::string> _triggerMap;
    std::list<std::string> _triggerConnections;
    std::list<std::string> _prefix;
    unsigned _networkCount;
    unsigned _triggerCount;

    std::string prefix() { return _prefix.back(); }
    void pushPrefix(const std::string& prefix) { _prefix.push_back(prefix); }
    void popPrefix() { _prefix.pop_back(); }
  };

} // namespace ChimeraTK
