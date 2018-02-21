#include "DummyBackend.h"
#include "BackendFactory.h"
#include "DeviceAccessVersion.h"

namespace mtca4u{
  using namespace ChimeraTK;
}
using namespace mtca4u;

struct WorkingBackend : public DummyBackend{
  using DummyBackend::DummyBackend;
  
  static boost::shared_ptr<DeviceBackend> createInstance(std::string /*host*/, std::string instance, std::list<std::string> parameters, std::string /*mapFileName*/){
    return returnInstance<WorkingBackend>(instance, convertPathRelativeToDmapToAbs(parameters.front()));
  }

  struct BackendRegisterer{
    BackendRegisterer(){
      mtca4u::BackendFactory::getInstance().registerBackendType("working","",&WorkingBackend::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
    }
  };

};

static WorkingBackend::BackendRegisterer gWorkingBackendRegisterer;

extern "C"{
  const char * deviceAccessVersionUsedToCompile(){
    return CHIMERATK_DEVICEACCESS_VERSION;
  }
}
