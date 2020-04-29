#include "StatusAggregator.h"

#include <list>
#include <regex>

namespace ChimeraTK {

  void StatusAggregator::populateStatusInput() {
    std::cout << "Populating aggregator " << getName() << ", fully qualified name is: " << getQualifiedName()
              << std::endl;

    // This does not work, we need to operate on the virtual hierarchy
    //std::list<Module*> subModuleList{getOwner()->getSubmoduleList()};

    // This approach crashes
    //    auto subModuleList = getOwner()->findTag(".*").getSubmoduleList();

    //    for(auto module : subModuleList){

    //      std::cout << "Testing Module " << module->getName() << std::endl;
    //      auto accList{module->getAccessorList()};
    //    }

    // Another try, virtualise the entire Application
    auto virtualisedApplication = Application::getInstance().findTag(".*");
    auto virtualPathToThis = getVirtualQualifiedName();
    std::cout << "  ** Fully qualified name in vitual hierarchy: "
              << getVirtualQualifiedName() << std::endl;


    // Remove application name and name of leaf node from the path
    auto pathBegin = virtualPathToThis.find_first_not_of("/" + Application::getInstance().getName() + "/");
    auto pathEnd = virtualPathToThis.find_last_of("/") - 1;

    auto pathWithoutAppAndLeafNode{virtualPathToThis.substr(pathBegin, pathEnd - pathBegin + 1)};

    std::list<VariableNetworkNode> allAccessors;
    std::list<Module*> allSubmodules;
    if(pathWithoutAppAndLeafNode == getName()) {
      // Path to this module, the parent is the Application
      allAccessors = virtualisedApplication.getAccessorListRecursive();
      allSubmodules = virtualisedApplication.getSubmoduleListRecursive();

      std::cout << "Parent is the Application" << std::endl;
    }
    else {
      std::cout << "SubPath to this aggregator's parent : " << pathWithoutAppAndLeafNode << std::endl;

      Module& virtualParent =
          virtualisedApplication.submodule(virtualPathToThis.substr(pathBegin, pathEnd - pathBegin + 1));

      virtualParent.dump();

      allAccessors = virtualParent.getAccessorListRecursive();
      allSubmodules = virtualParent.getSubmoduleListRecursive();
    }

    // Works, as long as we do not use HIerarchyModifiers other than "none" in the test app
    // Update: This still has problems, because getOwner gives us the "real" C++ Module
    //          We need to do everything on the virtual plane
    //auto allAccessors{virtualParent.getAccessorListRecursive()};
    //auto allSubmodules{virtualParent.getSubmoduleListRecursive()};

    std::cout << "  Size of allAccessors: " << allAccessors.size() << std::endl;
    std::cout << "  Size of allSubmodules: " << allSubmodules.size() << std::endl;

    for(auto acc : allAccessors) {
      if(acc.getDirection().dir == VariableDirection::feeding) {
        std::cout << "      -- Accessor: " << acc.getName() << " of module: " << acc.getOwningModule()->getName()
                  << std::endl;
      }
    }

    //      if(dynamic_cast<StatusMonitor*>(module)){
    //        std::cout << "  Found Monitor " << module->getName() << std::endl;
    //      }
    //      else if(dynamic_cast<StatusAggregator*>(module)){
    //          std::string moduleName = module->getName();

    //          std::cout << "Found Aggregator " << moduleName << std::endl;

    //          statusInput.emplace_back(this, moduleName, "", "");
    //      }
    //      else if(dynamic_cast<ModuleGroup*>(module)){
    //        std::cout << "Found ModuleGroup " << module->getName() << std::endl;
    //      }
    //      else{}
    //    }
    std::cout << std::endl << std::endl;
  } // poplateStatusInput()*/
} // namespace ChimeraTK
