#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/DeviceAccessVersion.h>
#include <ChimeraTK/DummyBackend.h>

class ExceptionDummy : public ChimeraTK::DummyBackend {
 public:
  ExceptionDummy(std::string mapFileName) : DummyBackend(mapFileName) {}
  bool throwExceptionOpen{false};
  bool throwExceptionRead{false};
  bool throwExceptionWrite{false};
  static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string> parameters) {
    return boost::shared_ptr<DeviceBackend>(new ExceptionDummy(parameters["map"]));
  }

  void open() override {
    if(throwExceptionOpen) {
      throw(ChimeraTK::runtime_error("DummyException: This is a test"));
    }
    ChimeraTK::DummyBackend::open();
    if(throwExceptionRead || throwExceptionWrite) {
      throw(ChimeraTK::runtime_error("DummyException: open throws because of device error when already open."));
    }
  }

  void read(uint8_t bar, uint32_t address, int32_t* data, size_t sizeInBytes) override {
    if(throwExceptionRead) {
      throw(ChimeraTK::runtime_error("DummyException: read throws by request"));
    }
    ChimeraTK::DummyBackend::read(bar, address, data, sizeInBytes);
  }

  void write(uint8_t bar, uint32_t address, int32_t const* data, size_t sizeInBytes) override {
    if(throwExceptionWrite) {
      throw(ChimeraTK::runtime_error("DummyException: write throws by request"));
    }
    ChimeraTK::DummyBackend::write(bar, address, data, sizeInBytes);
  }

  bool isFunctional() const override {
    return (_opened && !throwExceptionOpen && !throwExceptionRead && !throwExceptionWrite);
  }

  class BackendRegisterer {
   public:
    BackendRegisterer();
  };
  static BackendRegisterer backendRegisterer;
};

ExceptionDummy::BackendRegisterer ExceptionDummy::backendRegisterer;

ExceptionDummy::BackendRegisterer::BackendRegisterer() {
  std::cout << "ExceptionDummy::BackendRegisterer: registering backend type "
               "ExceptionDummy"
            << std::endl;
  ChimeraTK::BackendFactory::getInstance().registerBackendType("ExceptionDummy", &ExceptionDummy::createInstance);
}
