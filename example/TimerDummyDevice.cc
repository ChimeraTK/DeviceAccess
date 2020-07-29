#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/DeviceAccessVersion.h>
#include <ChimeraTK/DeviceBackendImpl.h>
#include <ChimeraTK/NDRegisterAccessor.h>

template<typename UserType>
class TimerDummyRegisterAccessor;

class TimerDummy : public ChimeraTK::DeviceBackendImpl {
 public:
  TimerDummy() : DeviceBackendImpl() { FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl); }

  static boost::shared_ptr<DeviceBackend> createInstance(
      std::string, std::string, std::list<std::string>, std::string) {
    return boost::shared_ptr<DeviceBackend>(new TimerDummy());
  }

  template<typename UserType>
  boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> getRegisterAccessor_impl(
      const ChimeraTK::RegisterPath& registerPathName, size_t, size_t, ChimeraTK::AccessModeFlags flags);
  DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(TimerDummy, getRegisterAccessor_impl, 4);

  void open() override {}

  void close() override {}

  bool isFunctional() const override { return true; }

  void setException() override {}

  std::string readDeviceInfo() override { return std::string("Dummy timing device "); }

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
  ChimeraTK::BackendFactory::getInstance().registerBackendType(
      "TimerDummy", "", &TimerDummy::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
}

template<typename UserType>
class TimerDummyRegisterAccessor : public ChimeraTK::NDRegisterAccessor<UserType> {
 public:
  TimerDummyRegisterAccessor(const ChimeraTK::RegisterPath& registerPathName)
  : ChimeraTK::NDRegisterAccessor<UserType>(registerPathName, {ChimeraTK::AccessMode::wait_for_new_data}) {
    ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D.resize(1);
    ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D[0].resize(1);
    ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D[0][0] = UserType();

    this->_readQueue = {3};
  }

  ~TimerDummyRegisterAccessor() override {}

  void doReadTransferSynchronously() override { usleep(1000000); }

  void doPostRead(ChimeraTK::TransferType, bool hasNewData) override {
    if(!hasNewData) return;
    ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D[0][0]++;
    this->_versionNumber = {};
  }

  bool doWriteTransfer(ChimeraTK::VersionNumber) override { return false; }

  bool isReadOnly() const override { return true; }
  bool isReadable() const override { return true; }
  bool isWriteable() const override { return false; }

  bool mayReplaceOther(const boost::shared_ptr<ChimeraTK::TransferElement const>&) const override { return false; }

  std::vector<boost::shared_ptr<ChimeraTK::TransferElement>> getHardwareAccessingElements() override {
    return {this->shared_from_this()};
  }

  void replaceTransferElement(boost::shared_ptr<ChimeraTK::TransferElement>) override {}

  std::list<boost::shared_ptr<ChimeraTK::TransferElement>> getInternalElements() override { return {}; }

 private:
  boost::thread _timerThread;
};

template<>
void TimerDummyRegisterAccessor<std::string>::doPostRead(ChimeraTK::TransferType, bool /*hasNewData*/) {}

template<typename UserType>
boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> TimerDummy::getRegisterAccessor_impl(
    const ChimeraTK::RegisterPath& registerPathName, size_t, size_t, ChimeraTK::AccessModeFlags flags) {
  assert(registerPathName == "/macropulseNr");
  flags.checkForUnknownFlags({ChimeraTK::AccessMode::wait_for_new_data});
  return boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>>(
      new TimerDummyRegisterAccessor<UserType>(registerPathName));
}
