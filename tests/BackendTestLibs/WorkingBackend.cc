#include "DummyBackend.h"
#include "BackendFactory.h"

namespace ChimeraTK{
  using namespace ChimeraTK;
}
using namespace ChimeraTK;

struct WorkingBackend : public DummyBackend{
  using DummyBackend::DummyBackend;

  static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string,std::string> parameters) {
    return returnInstance<WorkingBackend>(parameters.at("map"), convertPathRelativeToDmapToAbs(parameters.at("map")));
  }

  struct BackendRegisterer{
    BackendRegisterer(){
      ChimeraTK::BackendFactory::getInstance().registerBackendType("working",&WorkingBackend::createInstance,{"map"});
    }
  };

};

static WorkingBackend::BackendRegisterer gWorkingBackendRegisterer;

extern "C"{
  const char * deviceAccessVersionUsedToCompile(){
    return CHIMERATK_DEVICEACCESS_VERSION;
  }
}
