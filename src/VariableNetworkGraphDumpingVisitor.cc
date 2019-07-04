#include "VariableNetworkGraphDumpingVisitor.h"
#include "Application.h"
#include "VariableNetwork.h"
#include "VisitorHelper.h"

#include <algorithm>
#include <sstream>
#include <typeinfo>

namespace ChimeraTK {

  void VariableNetworkGraphDumpingVisitor::dispatch(const VariableNetwork& network) {
    std::string networkPrefix = "network_" + std::to_string(_networkCount++);
    pushPrefix(networkPrefix);

    stream() << "  subgraph cluster_" << prefix() << " {\n"
             << "    fontsize=\"8\";\n"
             << "    style=\"filled,rounded\";\n"
             << "    color=black;\n"
             << "    fillcolor=white;\n"
             << "    ordering=out;\n"
             << "    label=\"" << boost::core::demangle(typeid(network).name()) << "(" << &network << ")\\n"
             << network.getDescription() << "\\n"
             << "value type = " << boost::core::demangle(network.getValueType().name()) << "\\n"
             << "engineering unit = " << network.getUnit() << "\";\n";

    std::string feeder;
    std::string trigger;
    if(network.hasFeedingNode()) {
      auto feederNode = network.getFeedingNode();
      // We are inside a trigger network... Consumers will be skipped below. Make
      // the feeder a "global" variable
      if(!network.getConsumingNodes().empty() &&
          network.getConsumingNodes().begin()->getType() == NodeType::TriggerReceiver) {
        feeder = detail::encodeDotNodeName(detail::nodeName(feederNode));
        if(_triggerMap.find(feeder) == _triggerMap.end()) {
          std::stringstream ss;
          pushStream(ss);
          pushPrefix("");
          feederNode.accept(*this);
          _triggerMap[feeder] = ss.str();
          popPrefix();
          popStream();
        }
      }
      else {
        feeder = prefix() + detail::encodeDotNodeName(detail::nodeName(feederNode));
        feederNode.accept(*this);
      }

      if(feederNode.hasExternalTrigger()) {
        auto triggerNode = feederNode.getExternalTrigger();
        trigger = detail::encodeDotNodeName(detail::nodeName(triggerNode));
        if(_triggerMap.find(trigger) == _triggerMap.end()) {
          std::stringstream ss;
          pushStream(ss);
          pushPrefix("");
          triggerNode.accept(*this);
          _triggerMap[trigger] = ss.str();
          popPrefix();
          popStream();
        }
      }
    }

    for(auto& consumerNode : network.getConsumingNodes()) {
      if(consumerNode.getDirection().dir != VariableDirection::consuming) continue;

      if(consumerNode.getType() == NodeType::TriggerReceiver) continue;

      auto consumer = prefix() + detail::encodeDotNodeName(detail::nodeName(consumerNode));
      consumerNode.accept(*this);
      std::string helperNode;

      if(!trigger.empty()) {
        // Create trigger connection diamond
        _triggerCount++;
        helperNode = prefix() + "_trigger_helper_" + std::to_string(_triggerCount);
        stream() << helperNode
                 << "[label=\"\",shape=diamond,style=\"filled\",color=black,"
                    "width=.3,height=.3,fixedsize=true,fillcolor=\"#ffcc00\"]\n";
      }

      stream() << feeder << " -> ";
      if(!helperNode.empty()) {
        stream() << helperNode << " -> " << consumer << "\n";
        // Hack: Make trigger lower in rank than all entry points to subgraphs
        _triggerConnections.push_back(trigger + " -> " + feeder + "[style = invis]\n");

        _triggerConnections.push_back(trigger + " -> " + helperNode + "[style = dashed, color=grey,tailport=s]\n");
      }
      else {
        stream() << consumer << "\n";
      }
    }

    stream() << "}\n";
    popPrefix();
  }

  VariableNetworkGraphDumpingVisitor::VariableNetworkGraphDumpingVisitor(std::ostream& stream)
  : Visitor<Application, VariableNetwork>(), VariableNetworkNodeDumpingVisitor(stream, "\\n"), _triggerMap(),
    _triggerConnections(), _networkCount(0), _triggerCount(0) {}

  void VariableNetworkGraphDumpingVisitor::dispatch(const Application& t) {
    stream() << "digraph application {\n"
             //<< "  rankdir = LR;\n"
             << "  fontname=\"Sans\";\n"
             << "  fontsize=\"10\";\n"
             << "  labelloc=t;\n"
             << "  nodesep=1;\n"
             //<< "  splines=ortho;\n"
             << "  concentrate=true;\n"
             << "  label=\"<" << boost::core::demangle(typeid(t).name()) << ">" << t.getName() << "\";\n"
             << "  node [style=\"filled,rounded\", shape=box, fontsize=\"9\", "
                "fontname=\"sans\"];\n"
             << "  edge [labelfontsize=\"6\", fontsize=\"9\", "
                "fontname=\"monospace\"];\n"
             << "  \n";

    for(auto& network : t.networkList) {
      network.accept(*this);
    }

    for(auto& mapElem : _triggerMap) {
      stream() << mapElem.second;
    }

    for(auto& c : _triggerConnections) {
      stream() << c;
    }

    stream() << "}\n";
  }

  void VariableNetworkGraphDumpingVisitor::dispatch(const VariableNetworkNode& t) {
    std::string nodeName = prefix() + detail::encodeDotNodeName(detail::nodeName(t));
    stream() << nodeName << "[\n";
    if(t.getMode() == UpdateMode::push)
      stream() << "    fillcolor=\"#ff9900\";\n";
    else if(t.getMode() == UpdateMode::poll)
      stream() << "    fillcolor=\"#b4d848\";\n";
    else {
      stream() << "    fillcolor=\"#ffffff\";\n";
    }

    stream() << "    label=\"" << detail::nodeName(t) << "\\n";

    VariableNetworkNodeDumpingVisitor::dispatch(t);

    stream() << "\"]\n";
  }
} // namespace ChimeraTK
