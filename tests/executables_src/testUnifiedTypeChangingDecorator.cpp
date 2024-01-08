// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE TypeChangingDecoratorUnifiedTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BackendFactory.h"
#include "Device.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"
#include "ExceptionDummyBackend.h"
#include "TransferGroup.h"
#include "TypeChangingDecorator.h"
#include "UnifiedBackendTest.h"

namespace ChimeraTK {
  using namespace ChimeraTK;
}
using namespace ChimeraTK;

// Create a test suite which holds all your tests.
BOOST_AUTO_TEST_SUITE(TypeChangingDecoratorUnifiedTest)

/**********************************************************************************************************************/

std::pair<RegisterPath, DecoratorType> getPathAndType(RegisterPath path) {
  DecoratorType type;
  path.setAltSeparator(".");
  auto typeName = path.getComponents().back();
  if(typeName == "casted") {
    type = DecoratorType::C_style_conversion;
  }
  else if(typeName == "limiting") {
    type = DecoratorType::limiting;
  }
  else {
    throw ChimeraTK::logic_error("Decorator type " + typeName + " not supported");
  }

  // drop last component
  path--;

  return {path, type};
}

/**********************************************************************************************************************/

class DecoratorBackend : public ExceptionDummy {
 public:
  DecoratorBackend(std::string mapFileName) : ExceptionDummy(mapFileName) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
  }

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    if(flags.has(AccessMode::raw)) throw ChimeraTK::logic_error("Raw accessors not supported");

    auto [path, type] = getPathAndType(registerPathName);

    return getTypeChangingDecorator<UserType>(
        ExceptionDummy::getRegisterAccessor_impl<float>(path, numberOfWords, wordOffsetInRegister, flags), type);
  }

  static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string> parameters) {
    return boost::shared_ptr<DeviceBackend>(new DecoratorBackend(parameters["map"]));
  }

  class BackendRegisterer {
   public:
    BackendRegisterer();
  };
  static BackendRegisterer backendRegisterer;
};

/**********************************************************************************************************************/

template<>
boost::shared_ptr<NDRegisterAccessor<std::string>> DecoratorBackend::getRegisterAccessor_impl(
    const RegisterPath&, size_t, size_t, AccessModeFlags) {
  std::terminate();
}

/********************************************************************************************************************/

DecoratorBackend::BackendRegisterer DecoratorBackend::backendRegisterer;

/********************************************************************************************************************/

DecoratorBackend::BackendRegisterer::BackendRegisterer() {
  std::cout << "DecoratorBackend::BackendRegisterer: registering backend type DecoratorBackend" << std::endl;
  ChimeraTK::BackendFactory::getInstance().registerBackendType("DecoratorBackend", &DecoratorBackend::createInstance);
}
/**********************************************************************************************************************/

static std::string cdd("(DecoratorBackend:1?map=decoratorTest.map)");
static auto exceptionDummy =
    boost::dynamic_pointer_cast<DecoratorBackend>(BackendFactory::getInstance().createBackend(cdd));

/**********************************************************************************************************************/

template<typename T>
struct TestRegister {
  bool isWriteable() { return true; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {}; }
  size_t nChannels() { return 1; }
  size_t nElementsPerChannel() { return 1; }
  size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  size_t nRuntimeErrorCases() { return 1; }
  typedef T minimumUserType;
  typedef minimumUserType rawUserType;

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableTestWriteNeverLosesData()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestCatalogue();

  // We cheat a bit here for the ro accessors. SCALA_RO is mapped to the same address
  // in the map file
  DummyRegisterAccessor<float> acc{exceptionDummy.get(), "", "/SOME/SCALAR"};

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    UserType val;
    // wrap around when we overflow
    try {
      val = numericToUserType<UserType>(acc + 3);
    }
    catch(boost::numeric::positive_overflow&) {
      val = std::numeric_limits<UserType>::min();
    }
    catch(boost::numeric::negative_overflow&) {
      val = std::numeric_limits<UserType>::max();
    }

    return {{val}};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    return {{numericToUserType<UserType>(static_cast<double>(acc))}};
  }

  void setRemoteValue() { acc = generateValue<minimumUserType>()[0][0]; }

  void setForceRuntimeError(bool enable, size_t) {
    exceptionDummy->throwExceptionRead = enable;
    exceptionDummy->throwExceptionWrite = enable;
  }
};

/**********************************************************************************************************************/

template<typename T>
struct TestRegisterRoCasted : TestRegister<T> {
  std::string path() { return "/SOME/SCALAR_RO/casted"; }
  bool isWriteable() { return false; }
};

/**********************************************************************************************************************/

template<typename T>
struct TestRegisterCasted : TestRegister<T> {
  std::string path() { return "/SOME/SCALAR/casted"; }
};

/**********************************************************************************************************************/

template<typename T>
struct TestRegisterCastedAsync : TestRegister<T> {
  virtual std::string path() { return "/SOME/SCALAR/PUSH_READ/casted"; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {AccessMode::wait_for_new_data}; }

  void setRemoteValue() {
    TestRegister<T>::setRemoteValue();
    exceptionDummy->triggerPush(RegisterPath(path())--);
  }

  void setForceRuntimeError(bool enable, size_t) {
    TestRegister<T>::setForceRuntimeError(enable, 0);
    // For async variables we need to also trigger an async operation on the
    // target register
    if(enable) {
      exceptionDummy->triggerPush(RegisterPath(path())--);
    }
  }
};

/**********************************************************************************************************************/

template<typename T>
struct TestRegisterCastedAsyncRo : TestRegisterCastedAsync<T> {
  std::string path() override { return "/SOME/SCALAR_RO/PUSH_READ/casted"; }
  bool isWriteable() { return false; }
};

/**********************************************************************************************************************/

template<typename T>
struct TestRegisterRangeChecked : TestRegister<T> {
  std::string path() { return "/SOME/SCALAR/limiting"; }
};

/**********************************************************************************************************************/

template<typename T>
struct TestRegisterRoRangeChecked : TestRegister<T> {
  std::string path() { return "/SOME/SCALAR_RO/limiting"; }
  bool isWriteable() { return false; }
};

/**********************************************************************************************************************/
/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRegisterAccessor) {
  std::cout << "*** testRegisterAccessor *** " << std::endl;
  ChimeraTK::UnifiedBackendTest<>()
      .testOnlyTransferElement()
      .addRegister<TestRegisterCasted<int64_t>>()
      .addRegister<TestRegisterCasted<double>>()
      .addRegister<TestRegisterRoCasted<int64_t>>()
      .addRegister<TestRegisterRoCasted<double>>()
      .addRegister<TestRegisterCastedAsync<int64_t>>()
      .addRegister<TestRegisterCastedAsync<double>>()
      .addRegister<TestRegisterCastedAsyncRo<int64_t>>()
      .addRegister<TestRegisterCastedAsyncRo<double>>()
      .addRegister<TestRegisterRangeChecked<int32_t>>()
      .addRegister<TestRegisterRangeChecked<float>>()
      .addRegister<TestRegisterRoRangeChecked<int32_t>>()
      .addRegister<TestRegisterRoRangeChecked<float>>()
      .runTests(cdd);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
