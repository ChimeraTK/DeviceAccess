#include "BackendFactory.h"
#include "DummyBackend.h"

namespace ChimeraTK {
  using namespace ChimeraTK;
}
using namespace ChimeraTK;

struct DummyForAreaHandshakeBackend : public DummyBackend {
  using DummyBackend::DummyBackend;

  static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string> parameters) {
    return returnInstance<DummyForAreaHandshakeBackend>(
        parameters.at("map"), convertPathRelativeToDmapToAbs(parameters.at("map")));
  }

  struct BackendRegisterer {
    BackendRegisterer() {
      ChimeraTK::BackendFactory::getInstance().registerBackendType(
          "dummyForAreaHandshake", &DummyForAreaHandshakeBackend::createInstance, {"map"});
    }
  };

  void write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) {
    setBusy();
    DummyBackend::write(bar, address, data, sizeInBytes);
  };
  void setBusy() {
    // APP.1.STATUS of tests/SubdeviceTarget.map
    uint64_t address = 8;
    int32_t data = 1;
    DummyBackend::write(1, address, &data, 4);
  }
};

static DummyForAreaHandshakeBackend::BackendRegisterer gDFAHBackendRegisterer;
