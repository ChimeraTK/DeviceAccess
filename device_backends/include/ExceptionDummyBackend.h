#include "BackendFactory.h"
#include "DeviceAccessVersion.h"
#include "DummyBackend.h"

namespace ChimeraTK {
  class ExceptionDummy : public ChimeraTK::DummyBackend {
   public:
    ExceptionDummy(std::string mapFileName) : DummyBackend(mapFileName) {}
    std::atomic<bool> throwExceptionOpen{false};
    std::atomic<bool> throwExceptionRead{false};
    std::atomic<bool> throwExceptionWrite{false};
    std::atomic<bool> thereHaveBeenExceptions{false};

    static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string> parameters) {
      return boost::shared_ptr<DeviceBackend>(new ExceptionDummy(parameters["map"]));
    }

    void open() override {
      if(throwExceptionOpen) {
        thereHaveBeenExceptions = true;
        throw(ChimeraTK::runtime_error("DummyException: This is a test"));
      }
      ChimeraTK::DummyBackend::open();
      if(throwExceptionRead || throwExceptionWrite) {
        thereHaveBeenExceptions = true;
        throw(ChimeraTK::runtime_error("DummyException: open throws because of "
                                       "device error when already open."));
      }
      thereHaveBeenExceptions = false;
    }

    void read(uint8_t bar, uint32_t address, int32_t* data, size_t sizeInBytes) override {
      if(throwExceptionRead) {
        thereHaveBeenExceptions = true;
        throw(ChimeraTK::runtime_error("DummyException: read throws by request"));
      }
      ChimeraTK::DummyBackend::read(bar, address, data, sizeInBytes);
    }

    void write(uint8_t bar, uint32_t address, int32_t const* data, size_t sizeInBytes) override {
      if(throwExceptionWrite) {
        thereHaveBeenExceptions = true;
        throw(ChimeraTK::runtime_error("DummyException: write throws by request"));
      }
      ChimeraTK::DummyBackend::write(bar, address, data, sizeInBytes);
    }

    bool isFunctional() const override {
      // thereHaveBeenExceptions is different from _hasActiceException
      // * thereHaveBeenExceptions is set when this class originally raised an exception
      // * _hasActiveException is raised externally via setException. This can happen if a transfer element from another backend,
      //   which is in the same logical name mapping backend than this class, has seen an exception.
      return (_opened && !throwExceptionOpen && !throwExceptionRead && !throwExceptionWrite &&
          !thereHaveBeenExceptions && !_hasActiveException);
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
} // namespace ChimeraTK
