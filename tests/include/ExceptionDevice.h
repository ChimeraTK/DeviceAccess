#include <ChimeraTK/DummyBackend.h>
#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/DeviceAccessVersion.h>

class ExceptionDummy : public ChimeraTK::DummyBackend {
  public:
    ExceptionDummy(std::string mapFileName) : DummyBackend(mapFileName) {
      std::cout<<"ExceptionDummy::ExceptionDummy"<<std::endl;
      throwException = false;
      }
    bool throwException;
    static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::string, std::list<std::string> parameters, std::string) {
      return boost::shared_ptr<DeviceBackend>(new ExceptionDummy(parameters.front()));
    }
    void open()override{
      std::cout<<"ExceptionDummy opened called"<<std::endl;
      if (throwException)
        throw(ChimeraTK::runtime_error("DummyException: This is a test"));
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
    ChimeraTK::BackendFactory::getInstance().registerBackendType("ExceptionDummy","",&ExceptionDummy::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
}
