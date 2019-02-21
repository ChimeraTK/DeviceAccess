#include "BackendFactory.h"
#include "DeviceAccessVersion.h"
#include "DummyBackend.h"

using namespace ChimeraTK;

/// This backend does register, so loading the plugin will go wrong
struct NotRegisteringPlugin : public DummyBackend {
  using DummyBackend::DummyBackend;

  static boost::shared_ptr<DeviceBackend>
  createInstance(std::string /*host*/, std::string instance,
                 std::list<std::string> parameters,
                 std::string /*mapFileName*/) {
    return returnInstance<NotRegisteringPlugin>(
        instance, convertPathRelativeToDmapToAbs(parameters.front()));
  }
};
