#include "ModuleGraphVisitor.h"

#include "EntityOwner.h"
#include "Module.h"
#include "VariableNetworkNode.h"
#include "VisitorHelper.h"

namespace ChimeraTK {

ModuleGraphVisitor::ModuleGraphVisitor(std::ostream& stream, bool showVariables)
    : Visitor<ChimeraTK::EntityOwner, ChimeraTK::Module, ChimeraTK::VariableNetworkNode> ()
    , _stream(stream)
    , _showVariables(showVariables) {}

void ModuleGraphVisitor::dispatch(const EntityOwner &owner) {
    /* If we start with an entity owner, consider ourselves the start of a graph */
    /* When descending from here, we only use Module directly */
    _stream << "digraph G {" << "\n";
    dumpEntityOwner(owner);
    _stream << "}" << std::endl;
}

void ModuleGraphVisitor::dispatch(const VariableNetworkNode &node) {
    std::string dotNode = detail::encodeDotNodeName(node.getQualifiedName());
    _stream << "  " <<  dotNode << "[label=\"{" << node.getName() << "| {";
    bool first = true;
    for(auto tag : node.getTags()) {
      if(!first) {
        _stream << "|";
      }
      else {
        first = false;
      }
      _stream << tag;
    }
    _stream << "}}\", shape=record]" << std::endl;
}

void ModuleGraphVisitor::dispatch(const Module &module) {
    dumpEntityOwner(static_cast<const EntityOwner &>(module));
}

void ModuleGraphVisitor::dumpEntityOwner(const EntityOwner &module) {
    std::string myDotNode = detail::encodeDotNodeName(module.getQualifiedName());
    _stream << "  " << myDotNode << "[label=\"" << module.getName() << "\"";
    if(module.getEliminateHierarchy()) {
      _stream << ",style=dotted";
    }
    if(module.getModuleType() == EntityOwner::ModuleType::ModuleGroup) {
      _stream << ",peripheries=2";
    }
    if(module.getModuleType() == EntityOwner::ModuleType::ApplicationModule) {
      _stream << ",penwidth=3";
    }
    _stream << "]" << std::endl;

    if(_showVariables) {
      for(auto &node : module.getAccessorList()) {
          std::string dotNode = detail::encodeDotNodeName(detail::nodeName(node));
          node.accept(*this);
          _stream << "  " << myDotNode << " -> " << dotNode << std::endl;
      }
    }

    for(const Module *submodule : module.getSubmoduleList()) {
      if(submodule->getModuleType() == EntityOwner::ModuleType::Device ||
         submodule->getModuleType() == EntityOwner::ModuleType::ControlSystem) continue;
      std::string dotNode = detail::encodeDotNodeName(submodule->getQualifiedName());
      _stream << "  " << myDotNode <<  " -> " << dotNode << std::endl;
      submodule->accept(*this);
    }
}
}
