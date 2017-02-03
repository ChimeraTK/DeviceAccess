#include "DummyBackend.h"
#include "BackendFactory.h"
#include "DeviceAccessVersion.h"

using namespace mtca4u;
#define WRONG_VERSION "00.18"

struct WrongVersionBackend : public DummyBackend{
  using DummyBackend::DummyBackend;
  
  static boost::shared_ptr<DeviceBackend> createInstance(std::string /*host*/, std::string instance, std::list<std::string> parameters, std::string /*mapFileName*/){
    return returnInstance<WrongVersionBackend>(instance, convertPathRelativeToDmapToAbs(parameters.front()));
  }

  struct BackendRegisterer{
    BackendRegisterer(){
      mtca4u::BackendFactory::getInstance().registerBackendType("wrongVersionBackend","",&WrongVersionBackend::createInstance, WRONG_VERSION);
    }
  };

};

static WrongVersionBackend::BackendRegisterer gWrongVersionBackendRegisterer;


extern "C"{
  std::string versionUsedToCompile(){
    return WRONG_VERSION;
  }
}
