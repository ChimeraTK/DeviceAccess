#include "StatusAggregator.h"

#include <list>
#include <regex>

namespace ChimeraTK {

  void StatusAggregator::populateStatusInput() {
    std::cout << "Populating aggregator " << getName() << ", fully qualified name is: " << getQualifiedName()
              << std::endl;

    // Another try, virtualise the entire Application
    auto virtualisedApplication = Application::getInstance().findTag(".*");
    auto virtualPathToThis = getVirtualQualifiedName();

    // Remove application name and name of leaf node from the path
    auto pathBegin = virtualPathToThis.find_first_not_of("/" + Application::getInstance().getName() + "/");
    auto pathEnd = virtualPathToThis.find_last_of("/") - 1;

    auto pathWithoutAppAndLeafNode{virtualPathToThis.substr(pathBegin, pathEnd - pathBegin + 1)};

    std::list<VariableNetworkNode> allAccessors;
    std::list<Module*> allSubmodules;
    if(pathWithoutAppAndLeafNode == getName()) {
      // Path to this module, the parent is the Application
      allAccessors = virtualisedApplication.getAccessorList();
      allSubmodules = virtualisedApplication.getSubmoduleList();
    }
    else {
      Module& virtualParent =
          virtualisedApplication.submodule(virtualPathToThis.substr(pathBegin, pathEnd - pathBegin + 1));

      virtualParent.dump();

      allAccessors = virtualParent.getAccessorList();
      allSubmodules = virtualParent.getSubmoduleList();
    }

    std::cout << "  Size of allAccessors: " << allAccessors.size() << std::endl;
    std::cout << "  Size of allSubmodules: " << allSubmodules.size() << std::endl;

    // Two approaches to get the modules of interest to feed to scanAndPopulateFromHierarchyLeve:
    // 1. directly call getSubmoduleList on each level: gives VirtualModules and the dynamic_casts below fail
    // 2.use getAccessorList and call getOwningModule() on each Accessor (as done below): Does not find the status outputs right now
    //   Also, this means more effort to detect and recurse into ModuleGroups
    //for(auto acc : allAccessors) {
    //if(acc.getDirection().dir == VariableDirection::feeding) {
    //std::cout << "      -- Accessor: " << acc.getName() << " of module: " << acc.getOwningModule()->getName()
    //<< std::endl;
    //}

    scanAndPopulateFromHierarchyLevel(allAccessors);

    std::cout << std::endl << std::endl;
  } // poplateStatusInput()*/


  void StatusAggregator::scanAndPopulateFromHierarchyLevel(std::list<VariableNetworkNode> nodes) {
    if(nodes.empty()) {
      return;
    }

    bool statusAggregatorFound{false};
    std::list<Module*> instancesToBeAggregated;

    // This does loops per level:
    // 1. Find all StatusMonitors and StatusAggregators on this level, if there
    //    is an aggregator, we can discard the Monitors on this level
    // 2. Iterate over instancesToBeAggregated and add to statusInput
    for(auto node : nodes) {
      // Only check feeding nodes to test each Module only once on the status output
      if(node.getDirection().dir != VariableDirection::feeding){
        continue;
      }
      auto module{node.getOwningModule()};
      std::cout << "Scanning Module " << module->getName() << std::endl;


      if(dynamic_cast<StatusMonitor*>(module)) {
        std::cout << "  Found Monitor " << module->getName() << std::endl;
      }
      else if(dynamic_cast<StatusAggregator*>(module)) {
        std::string moduleName = module->getName();
        if(statusAggregatorFound){
          throw ChimeraTK::logic_error("StatusAggregator: A second instance was found on the same hierarchy level.");
        }
        statusAggregatorFound = true;

        std::cout << "Found Aggregator " << moduleName << std::endl;

        statusInput.emplace_back(this, moduleName, "", "");
      }
      else if(dynamic_cast<ModuleGroup*>(module)) {
        std::cout << "Found ModuleGroup " << module->getName() << std::endl;
        // Process level below
        scanAndPopulateFromHierarchyLevel(module->getAccessorList());
      }
      else {
      }
    }

    // 2. TODO Add status inputs
  }
} // namespace ChimeraTK
