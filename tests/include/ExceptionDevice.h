#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/DeviceAccessVersion.h>
#include <ChimeraTK/DummyBackend.h>

class ExceptionDummy : public ChimeraTK::DummyBackend {
 public:
  ExceptionDummy(std::string mapFileName) : DummyBackend(mapFileName) { throwException = false; }
  bool throwException;
  static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string> parameters) {
    return boost::shared_ptr<DeviceBackend>(new ExceptionDummy(parameters["map"]));
  }

  void open() override {
    if(throwException) {
      throw(ChimeraTK::runtime_error("DummyException: This is a test"));
    }
    ChimeraTK::DummyBackend::open();
  }

  void read(uint8_t bar, uint32_t address, int32_t* data, size_t sizeInBytes) override {
    if(throwException) {
      if(isOpen()) close();
      throw(ChimeraTK::runtime_error("DummyException: read throws by request"));
    }
    ChimeraTK::DummyBackend::read(bar, address, data, sizeInBytes);
  }

  void write(uint8_t bar, uint32_t address, int32_t const* data, size_t sizeInBytes) override {
    if(throwException) {
      if(isOpen()) close();
      throw(ChimeraTK::runtime_error("DummyException: write throws by request"));
    }
    ChimeraTK::DummyBackend::write(bar, address, data, sizeInBytes);
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
