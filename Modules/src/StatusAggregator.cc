#include "StatusAggregator.h"

#include <list>

namespace ChimeraTK{

  void StatusAggregator::populateStatusInput(){


    // This does not work, we need to operate on the virtual hierarchy
    //std::list<Module*> subModuleList{getOwner()->getSubmoduleList()};

    // This approach crashes
//    auto subModuleList = getOwner()->findTag(".*").getSubmoduleList();

//    for(auto module : subModuleList){

//      std::cout << "Testing Module " << module->getName() << std::endl;
//      auto accList{module->getAccessorList()};
//    }


    // Works, as long as we do not use HIerarchyModifiers other than "none" in the test app
    std::cout << "Populating aggregator " << getName() << std::endl;

    auto allAccessors{getOwner()->findTag(".*").getAccessorListRecursive()};

    std::cout <<  "  Size of allAccessors: " << allAccessors.size() << std::endl;

    for(auto acc : allAccessors){

      if(acc.getDirection().dir == VariableDirection::feeding){
        std::cout << "      -- Accessor: " << acc.getName()
                  << " of module: " << acc.getOwningModule()->getName()
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
}
