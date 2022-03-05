#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SubdeviceBackendUnifiedTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "ExceptionDummyBackend.h"
#include "UnifiedBackendTest.h"
#include "DummyRegisterAccessor.h"
#include "DummyBackend.h"
#include "LogicalNameMappingBackend.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(DoubleBufferingBackendUnifiedTestSuite)

/**********************************************************************************************************************/

static std::string rawDeviceCdd("(ExceptionDummy?map=doubleBuffer.map)");
static auto backdoor =
    boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(rawDeviceCdd));
//static boost::shared_ptr<ExceptionDummy> exceptionDummy;

/**********************************************************************************************************************/

template<typename Register>
struct AreaType : Register {
  static uint32_t _currentBufferNumber;

  bool isWriteable() { return false; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {ChimeraTK::AccessMode::raw}; }
  size_t nChannels() { return 1; }
  size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  size_t nRuntimeErrorCases() { return 0; }

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .disableTestWriteNeverLosesData()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly();

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    auto values = this->getRemoteValue<typename Register::minimumUserType>();
    for(size_t i = 0; i < this->nChannels(); ++i)
      for(size_t j = 0; j < this->nElementsPerChannel(); ++j) {
        values[i][j] += this->increment * (i + j + 1);
      }
    return values;
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue(bool = false) {
    // For Variables we don't have a backdoor. We have to use the normal read and write
    // functions which are good enough. It seems like a self consistency test, but all
    // functionality the variable has to provide is that I can write something, and
    // read it back, which is tested with it.

    // We might have to open the backend to perform the operation. We have to remember
    // that we did so and close it again it we did. Some tests require the backend to be closed.

    auto currentBufferNumber = backdoor->getRegisterAccessor<uint32_t>("APP.1.WORD_DUB_BUF_CURR", 0, 0, {});
    auto buffer0 = backdoor->getRegisterAccessor<typename Register::minimumUserType>(
        "APP/0/DAQ0_BUF0", this->nElementsPerChannel(), 0, {});
    auto buffer1 = backdoor->getRegisterAccessor<typename Register::minimumUserType>(
        "APP/0/DAQ0_BUF1", this->nElementsPerChannel(), 0, {});

    bool deviceWasOpened = false;
    if(!backdoor->isOpen()) {
      backdoor->open();
      deviceWasOpened = true;
    }

    boost::shared_ptr<NDRegisterAccessor<typename Register::minimumUserType>> currentBuffer;

    currentBufferNumber->read();

    if(currentBufferNumber->accessData(0) == 1) {
      currentBuffer = buffer0;
    }
    else {
      currentBuffer = buffer1;
    }
    currentBuffer->read();
    std::vector<std::vector<UserType>> v;
    for(size_t i = 0; i < this->nChannels(); ++i) {
      v.push_back(std::vector<UserType>());
      for(size_t j = 0; j < this->nElementsPerChannel(); ++j) {
        v[i].push_back(currentBuffer->accessData(j));
      }
    }

    if(deviceWasOpened) {
      backdoor->close();
    }

    return v;
  }

  void setRemoteValue() {
    auto currentBufferNumber = backdoor->getRegisterAccessor<uint32_t>("APP.1.WORD_DUB_BUF_CURR", 0, 0, {});
    auto buffer0 = backdoor->getRegisterAccessor<typename Register::minimumUserType>(
        "APP/0/DAQ0_BUF0", this->nElementsPerChannel(), 0, {});
    auto buffer1 = backdoor->getRegisterAccessor<typename Register::minimumUserType>(
        "APP/0/DAQ0_BUF1", this->nElementsPerChannel(), 0, {});
    boost::shared_ptr<NDRegisterAccessor<typename Register::minimumUserType>> currentBuffer;

    bool deviceWasOpened = false;
    if(!backdoor->isOpen()) {
      backdoor->open();
      deviceWasOpened = true;
    }

    currentBufferNumber->accessData(0) = _currentBufferNumber;
    currentBufferNumber->write();
    _currentBufferNumber = _currentBufferNumber ? 0 : 1; // change current buffer no. 0->1 or 1->0

    auto values = this->generateValue<typename Register::minimumUserType>();

    if(currentBufferNumber->accessData(0) == 1) {
      currentBuffer = buffer0;
    }
    else {
      currentBuffer = buffer1;
    }
    for(size_t i = 0; i < this->nChannels(); ++i) {
      for(size_t j = 0; j < this->nElementsPerChannel(); ++j) {
        currentBuffer->accessData(i, j) = values[i][j];
      }
    }
    currentBuffer->write();

    if(deviceWasOpened) {
      backdoor->close();
    }
  }

  void setForceRuntimeError(bool /*enable*/, size_t) {
    //    target->throwExceptionRead = enable;
    //    target->throwExceptionWrite = enable;
    assert(false);
  }
};

/*********************************************************************************************************************/

struct MyArea1 {
  std::string path() { return "/doubleBuffer"; }
  size_t nElementsPerChannel() { return 10; }
  size_t address() { return 20; }
  //uint32_t toRaw(float v) { return v * 65536.F; }
  //float fromRaw(uint32_t v) { return v / 65536.F; }
  //float limitGenerated(float e) {
  //  while(e > 32768.F) e -= 65535.F;
  //  while(e < -32767.F) e += 65535.F;
  //  return e;
  //}
  int32_t increment = 3;

  typedef uint32_t minimumUserType;
  typedef int32_t rawUserType;
};

/*********************************************************************************************************************/

template<typename Register>
uint32_t AreaType<Register>::_currentBufferNumber = 0;

BOOST_AUTO_TEST_CASE(testUnified) {
  //"(logicalNameMap?map=doubleBuffer.xlmap&target=(ExceptionDummy?map=doubleBuffer.map))";
  std::string lmap = "(logicalNameMap?map=doubleBuffer.xlmap&target=" + rawDeviceCdd + ")";

  //exceptionDummy =
  //    boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(rawDeviceCdd));

  ChimeraTK::UnifiedBackendTest<>().addRegister<AreaType<MyArea1>>().runTests(lmap);
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
