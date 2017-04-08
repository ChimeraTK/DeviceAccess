#include "DummyBackend.h"
#include "BackendFactory.h"
#include "DeviceAccessVersion.h"

using namespace mtca4u;

/// This backend does not have the deviceAccessVersionUsedToCompile symbol, needed to load at runtime.
/// It has, however, a working backend registerer.
struct NoSymbolBackend : public DummyBackend{
  using DummyBackend::DummyBackend;
  
  static boost::shared_ptr<DeviceBackend> createInstance(std::string /*host*/, std::string instance, std::list<std::string> parameters, std::string /*mapFileName*/){
    return returnInstance<NoSymbolBackend>(instance, convertPathRelativeToDmapToAbs(parameters.front()));
  }

  struct BackendRegisterer{
    BackendRegisterer(){
      mtca4u::BackendFactory::getInstance().registerBackendType("noSymbol","",&NoSymbolBackend::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
    }
  };

};

static NoSymbolBackend::BackendRegisterer gNoSymbolBackendRegisterer;
