#include <ChimeraTK/DeviceBackendImpl.h>
#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/DeviceAccessVersion.h>
#include <ChimeraTK/SyncNDRegisterAccessor.h>

template<typename UserType>
class TimerDummyRegisterAccessor;

class TimerDummy : public ChimeraTK::DeviceBackendImpl {
  public:
    TimerDummy() : DeviceBackendImpl() {
      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
    }

    static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::string, std::list<std::string>, std::string) {
      return boost::shared_ptr<DeviceBackend>(new TimerDummy());
    }

    template<typename UserType>
    boost::shared_ptr< ChimeraTK::NDRegisterAccessor<UserType> > getRegisterAccessor_impl(
        const ChimeraTK::RegisterPath &registerPathName, size_t , size_t , ChimeraTK::AccessModeFlags flags);
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
    ChimeraTK::BackendFactory::getInstance().registerBackendType("TimerDummy","",&TimerDummy::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
}

template<typename UserType>
class TimerDummyRegisterAccessor : public ChimeraTK::SyncNDRegisterAccessor<UserType> {
  public:
    TimerDummyRegisterAccessor(const ChimeraTK::RegisterPath &registerPathName)
    : ChimeraTK::SyncNDRegisterAccessor<UserType>(registerPathName)
    {
      ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D.resize(1);
      ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D[0].resize(1);
      ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D[0][0] = UserType();
    }

    ~TimerDummyRegisterAccessor() { this->shutdown(); }

    void doReadTransfer() override {
      usleep(1000000);
    }

    void doPostRead() override {
      ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D[0][0]++;
    }

    bool doWriteTransfer(ChimeraTK::VersionNumber) override { return false; }

    bool doReadTransferNonBlocking() override { return false; }

    bool doReadTransferLatest() override { return false; }
    bool isReadOnly() const override { return true; }
    bool isReadable() const override { return true; }
    bool isWriteable() const override { return false; }

    ChimeraTK::AccessModeFlags getAccessModeFlags() const override { return {ChimeraTK::AccessMode::wait_for_new_data}; }

    bool mayReplaceOther(const boost::shared_ptr<ChimeraTK::TransferElement const> &) const override { return false; }

    std::vector<boost::shared_ptr<ChimeraTK::TransferElement> > getHardwareAccessingElements() override { return { this->shared_from_this() }; }

    void replaceTransferElement(boost::shared_ptr<ChimeraTK::TransferElement>) override {}

    std::list<boost::shared_ptr<ChimeraTK::TransferElement> > getInternalElements() override { return {}; }

};

template<>
void TimerDummyRegisterAccessor<std::string>::doPostRead() {
}


template<typename UserType>
boost::shared_ptr< ChimeraTK::NDRegisterAccessor<UserType> > TimerDummy::getRegisterAccessor_impl(
                    const ChimeraTK::RegisterPath &registerPathName, size_t , size_t , ChimeraTK::AccessModeFlags flags) {
    assert(registerPathName == "/macropulseNr");
    assert(flags.has(ChimeraTK::AccessMode::wait_for_new_data));
    flags.checkForUnknownFlags({ChimeraTK::AccessMode::wait_for_new_data});
    return boost::shared_ptr< ChimeraTK::NDRegisterAccessor<UserType> >(new TimerDummyRegisterAccessor<UserType>(registerPathName));
}
