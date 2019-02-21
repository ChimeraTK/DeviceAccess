#include <ChimeraTK/DummyBackend.h>
#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/DeviceAccessVersion.h>

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
    else
      ChimeraTK::DummyBackend::open();
  }

  class BackendRegisterer {
   public:
    BackendRegisterer();
  };
  static BackendRegisterer backendRegisterer;
};

ExceptionDummy::BackendRegisterer ExceptionDummy::backendRegisterer;

ExceptionDummy::BackendRegisterer::BackendRegisterer() {
  std::cout << "ExceptionDummy::BackendRegisterer: registering backend type ExceptionDummy" << std::endl;
  ChimeraTK::BackendFactory::getInstance().registerBackendType("ExceptionDummy", &ExceptionDummy::createInstance);
}
