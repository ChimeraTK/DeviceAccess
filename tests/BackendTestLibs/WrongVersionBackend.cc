#include "DummyBackend.h"
#include "BackendFactory.h"
#include "DeviceAccessVersion.h"

namespace ChimeraTK {
  using namespace ChimeraTK;
}
using namespace ChimeraTK;
#define WRONG_VERSION "00.18"

// LCOV_EXCL_START these lines cannot be reached because the backend cannot be registered, thus it is never instantiated
struct WrongVersionBackend : public DummyBackend {
  using DummyBackend::DummyBackend;
  static boost::shared_ptr<DeviceBackend> createInstance(
      std::string address, std::map<std::string, std::string> parameters) {
    return returnInstance<WrongVersionBackend>(address, convertPathRelativeToDmapToAbs(parameters["map"]));
  }

  // LCOV_EXCL_STOP The registerern and the version functions have to be called

  struct BackendRegisterer {
    BackendRegisterer() {
      ChimeraTK::BackendFactory::getInstance().registerBackendType(
          "wrongVersionBackend", &WrongVersionBackend::createInstance, {}, WRONG_VERSION);
    }
  };
};

static WrongVersionBackend::BackendRegisterer gWrongVersionBackendRegisterer;
