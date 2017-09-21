#include <mtca4u/DeviceBackendImpl.h>
#include <mtca4u/BackendFactory.h>
#include <mtca4u/DeviceAccessVersion.h>
#include <mtca4u/NDRegisterAccessor.h>

template<typename UserType>
class TimerDummyRegisterAccessor;

class TimerDummy : public mtca4u::DeviceBackendImpl {
  public:
    TimerDummy() : DeviceBackendImpl() {
      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
    }

    static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::string, std::list<std::string>, std::string) {
      return boost::shared_ptr<DeviceBackend>(new TimerDummy());
    }

    template<typename UserType>
    boost::shared_ptr< mtca4u::NDRegisterAccessor<UserType> > getRegisterAccessor_impl(
        const mtca4u::RegisterPath &registerPathName, size_t , size_t , mtca4u::AccessModeFlags flags);
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER( TimerDummy, getRegisterAccessor_impl, 4);

    void open() override {}

    void close() override {}

    std::string readDeviceInfo() override {
      return std::string("Dummy timing device ");
    }

    /** Class to register the backend type with the factory. */
    class BackendRegisterer {
      public:
        BackendRegisterer();
    };
    static BackendRegisterer backendRegisterer;
};


TimerDummy::BackendRegisterer TimerDummy::backendRegisterer;

TimerDummy::BackendRegisterer::BackendRegisterer() {
    std::cout << "TimerDummy::BackendRegisterer: registering backend type TimerDummy" << std::endl;
    mtca4u::BackendFactory::getInstance().registerBackendType("TimerDummy","",&TimerDummy::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
}

template<typename UserType>
class TimerDummyRegisterAccessor : public mtca4u::NDRegisterAccessor<UserType> {
  public:
    TimerDummyRegisterAccessor(const mtca4u::RegisterPath &registerPathName)
    : mtca4u::NDRegisterAccessor<UserType>(registerPathName)
    {
      mtca4u::NDRegisterAccessor<UserType>::buffer_2D.resize(1);
      mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0].resize(1);
      mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0][0] = UserType();
    }
    
    ~TimerDummyRegisterAccessor() { this->shutdown(); }
    
    void doReadTransfer() override {
      usleep(1000000);
    }
    
    void postRead() override {
      mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0][0]++;
    }
    
    bool write(ChimeraTK::VersionNumber) override { return false; }

    bool doReadTransferNonBlocking() override { return false; }

    bool doReadTransferLatest() override { return false; }
    bool isReadOnly() const override { return true; }
    bool isReadable() const override { return true; }
    bool isWriteable() const override { return false; }

    bool isSameRegister(const boost::shared_ptr<mtca4u::TransferElement const> &) const override { return false; }
    
    std::vector<boost::shared_ptr<mtca4u::TransferElement> > getHardwareAccessingElements() override { return { this->shared_from_this() }; }
    
    void replaceTransferElement(boost::shared_ptr<mtca4u::TransferElement>) override {}

};
    
template<>
void TimerDummyRegisterAccessor<std::string>::postRead() {
}


template<typename UserType>
boost::shared_ptr< mtca4u::NDRegisterAccessor<UserType> > TimerDummy::getRegisterAccessor_impl(
                    const mtca4u::RegisterPath &registerPathName, size_t , size_t , mtca4u::AccessModeFlags flags) {
    assert(registerPathName == "/macropulseNr");
    assert(flags.has(mtca4u::AccessMode::wait_for_new_data));
    flags.checkForUnknownFlags({mtca4u::AccessMode::wait_for_new_data});
    return boost::shared_ptr< mtca4u::NDRegisterAccessor<UserType> >(new TimerDummyRegisterAccessor<UserType>(registerPathName));
}
