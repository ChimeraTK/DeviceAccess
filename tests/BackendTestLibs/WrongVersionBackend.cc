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

  // No registerer here: It would already trigger an exception when trying to register
  // the creator function with the wrong version, which is tested separately.
  // Here we want to test the reaction of the loading function on the wrong version.
  // @todo FIXME: If we cannot catch the exception as it is
  // thrown inside the registerer, we never reach the point that we can cleanly prevent the loading
  // of a bad plugin and react gracefully.

};


extern "C"{
  std::string versionUsedToCompile(){
    return WRONG_VERSION;
  }
}
