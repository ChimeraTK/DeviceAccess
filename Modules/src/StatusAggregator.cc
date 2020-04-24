#include "StatusAggregator.h"

#include <list>

namespace ChimeraTK{

  void StatusAggregator::populateStatusInput(){

    std::cout << "Populating aggregator " << getName() << ", fully qualified name is: "
              <<  getQualifiedName() <<std::endl;

    // This does not work, we need to operate on the virtual hierarchy
    //std::list<Module*> subModuleList{getOwner()->getSubmoduleList()};

    // This approach crashes
//    auto subModuleList = getOwner()->findTag(".*").getSubmoduleList();

//    for(auto module : subModuleList){

//      std::cout << "Testing Module " << module->getName() << std::endl;
//      auto accList{module->getAccessorList()};
//    }


    // Works, as long as we do not use HIerarchyModifiers other than "none" in the test app
     // Update: This still has problems, because getOwner gives us the "real" C++ Module
    //          We need to do everything on the virtual plane
//    auto allAccessors{getOwner()->findTag(".*").getAccessorListRecursive()};

//    std::cout <<  "  Size of allAccessors: " << allAccessors.size() << std::endl;

//    for(auto acc : allAccessors){

//      if(acc.getDirection().dir == VariableDirection::feeding){
//        std::cout << "      -- Accessor: " << acc.getName()
//                  << " of module: " << acc.getOwningModule()->getName()
//                  << std::endl;
//      }

//    }


    // Another try, virtualise the entire Application
    auto virtualApplicationHierarchy = Application::getInstance().findTag(".*");
    std::cout << "  ** Fully qualified name in vitual hierarchy: "
              << virtualApplicationHierarchy.getQualifiedName() << std::endl;

    virtualApplicationHierarchy.virtualise();
    auto virtualizedModuleList = virtualApplicationHierarchy.getSubmoduleListRecursive();

    for(auto module : virtualizedModuleList){
      std::cout << "  ** " << module->getQualifiedName();
      if (module == this){
        std::cout << "   THIS IS THIS STATUS_AGGREGATOR!";
      }
      std::cout << std::endl;
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
}
