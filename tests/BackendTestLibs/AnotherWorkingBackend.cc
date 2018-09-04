#include "DummyBackend.h"
#include "BackendFactory.h"
#include "DeviceAccessVersion.h"

namespace ChimeraTK{
  using namespace ChimeraTK;
}
using namespace ChimeraTK;

struct AnotherWorkingBackend : public DummyBackend{
  using DummyBackend::DummyBackend;
  
  static boost::shared_ptr<DeviceBackend> createInstance(std::string /*host*/, std::string instance, std::list<std::string> parameters, std::string /*mapFileName*/){
    return returnInstance<AnotherWorkingBackend>(instance, convertPathRelativeToDmapToAbs(parameters.front()));
  }

  struct BackendRegisterer{
    BackendRegisterer(){
      ChimeraTK::BackendFactory::getInstance().registerBackendType("another","",&AnotherWorkingBackend::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
    }
  };

};

static AnotherWorkingBackend::BackendRegisterer gAnotherWorkingBackendRegisterer;

extern "C"{
  const char * deviceAccessVersionUsedToCompile(){
    return CHIMERATK_DEVICEACCESS_VERSION;
  }
}
