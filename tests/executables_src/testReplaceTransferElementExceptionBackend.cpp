// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

/**********************************************************************************************************************/
/* Tests for CR-015: a forwarded replaceTransferElement() must not propagate the exception backend to its target,     */
/* while a replacement must still make the chain carry the decorator's exception backend.                             */
/**********************************************************************************************************************/

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ReplaceTransferElementExceptionBackendTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BackendFactory.h"
#include "BitRangeAccessorDecorator.h"
#include "Device.h"
#include "DummyBackend.h"
#include "LNMBackendBitAccessor.h"
#include "LNMBackendChannelAccessor.h"
#include "LogicalNameMappingBackend.h"
#include "NDRegisterAccessor.h"
#include "SubArrayAccessorDecorator.h"

#include <boost/make_shared.hpp>

#include <atomic>
#include <cstdint>

using namespace ChimeraTK;

/**********************************************************************************************************************/

namespace {

  /** Decorator which counts setExceptionBackend() calls (per type, statically) and forwards them to the decorated
   *  accessor. Used as an instrumented target of the classes under test. The static counter is reset before a forwarded
   *  call and read afterwards. */
  template<typename UserType>
  class CountingDecorator : public NDRegisterAccessorDecorator<UserType, UserType> {
   public:
    explicit CountingDecorator(boost::shared_ptr<NDRegisterAccessor<UserType>> target)
    : NDRegisterAccessorDecorator<UserType, UserType>(std::move(target)) {}

    void setExceptionBackend(boost::shared_ptr<DeviceBackend> exceptionBackend) override {
      ++_counter;
      NDRegisterAccessorDecorator<UserType, UserType>::setExceptionBackend(std::move(exceptionBackend));
    }

    bool mayReplaceOther(const boost::shared_ptr<TransferElement const>& other) const override {
      if(this == other.get()) {
        return false;
      }
      return mayReplaceOthers;
    }

    inline static std::atomic<size_t> _counter{0};
    bool mayReplaceOthers{false};
  };

  /********************************************************************************************************************/

  /** Dummy backend whose accessors are wrapped in the CountingDecorator, so setExceptionBackend() calls on the created
   *  target can be observed. Registered as backend type "instrumentingDummy" and referenced through a dmap. */
  class InstrumentingBackend : public DummyBackend {
   public:
    DEFINE_VIRTUAL_FUNCTION_OVERRIDE_VTABLE(DummyBackendBase, getRegisterAccessor_impl,
        boost::shared_ptr<NDRegisterAccessor<T>>(const RegisterPath&, size_t, size_t, AccessModeFlags));

    explicit InstrumentingBackend(const std::string& mapFileName) : DummyBackend(mapFileName) {
      OVERRIDE_VIRTUAL_FUNCTION_TEMPLATE(DummyBackendBase, getRegisterAccessor_impl);
    }

    template<typename T>
    boost::shared_ptr<NDRegisterAccessor<T>> getRegisterAccessor_impl(const RegisterPath& registerPathName,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
      auto inner =
          DummyBackend::getRegisterAccessor_impl<T>(registerPathName, numberOfWords, wordOffsetInRegister, flags);
      return boost::make_shared<CountingDecorator<T>>(inner);
    }

    static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string> parameters) {
      return boost::make_shared<InstrumentingBackend>(parameters.at("map"));
    }
  };

  /********************************************************************************************************************/

  /// Expose the protected internals of the classes under test for instrumentation.
  template<typename UserType>
  class TestNDDecorator : public NDRegisterAccessorDecorator<UserType, UserType> {
   public:
    using NDRegisterAccessorDecorator<UserType, UserType>::NDRegisterAccessorDecorator;

    boost::shared_ptr<NDRegisterAccessor<UserType>>& target() { return this->_target; }
  };

  template<typename UserType>
  class TestLNMBitAccessor : public LNMBackendBitAccessor<UserType> {
   public:
    using LNMBackendBitAccessor<UserType>::LNMBackendBitAccessor;
    using LNMBackendBitAccessor<UserType>::_accessor;

    void callReplace(boost::shared_ptr<TransferElement> newElement) { this->replaceTransferElement(newElement); }
  };

  template<typename UserType>
  class TestLNMChannelAccessor : public LNMBackendChannelAccessor<UserType> {
   public:
    using LNMBackendChannelAccessor<UserType>::LNMBackendChannelAccessor;
    using LNMBackendChannelAccessor<UserType>::_accessor;

    void callReplace(boost::shared_ptr<TransferElement> newElement) { this->replaceTransferElement(newElement); }
  };

  /********************************************************************************************************************/

  NumericAddressedRegisterInfo makeBitRangeRegisterInfo() {
    NumericAddressedRegisterInfo regInfo(
        "/BitRange", 1, 0, sizeof(uint64_t), 0, 8, 0, false, NumericAddressedRegisterInfo::Access::READ_WRITE);
    regInfo.channels[0].bitOffset = 0;
    return regInfo;
  }

  /// Register the instrumenting backend type exactly once, before any dmap referencing it is opened.
  void registerInstrumentingBackend() {
    static bool registered = false;
    if(!registered) {
      BackendFactory::getInstance().registerBackendType(
          "instrumentingDummy", &InstrumentingBackend::createInstance, {"map"});
      registered = true;
    }
  }

  boost::shared_ptr<LogicalNameMappingBackend> openLNMBackend() {
    registerInstrumentingBackend();
    BackendFactory::getInstance().setDMapFilePath("replaceTest.dmap");
    auto device = boost::make_shared<Device>();
    device->open("LMAP");
    return boost::dynamic_pointer_cast<LogicalNameMappingBackend>(device->getBackend());
  }

} // namespace

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE(ReplaceTransferElementExceptionBackendTestSuite)

/**********************************************************************************************************************/
/* NDRegisterAccessorDecorator                                                                                        */
/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestNDDecoratorForwardingDoesNotPropagate) {
  auto instrumenting = boost::make_shared<InstrumentingBackend>("testSubArrayAccessorDecorator.map");
  auto target = instrumenting->getRegisterAccessor_impl<int32_t>(RegisterPath("/MY_ARRAY"), 0, 0, AccessModeFlags({}));

  auto deco = boost::make_shared<TestNDDecorator<int32_t>>(target);

  // a target which does not accept the incoming element -> the call must be forwarded
  auto other = boost::make_shared<TestNDDecorator<int32_t>>(
      instrumenting->getRegisterAccessor_impl<int32_t>(RegisterPath("/MY_ARRAY"), 0, 0, AccessModeFlags({})));

  CountingDecorator<int32_t>::_counter = 0;
  deco->replaceTransferElement(other);
  BOOST_TEST(CountingDecorator<int32_t>::_counter == 0);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestNDDecoratorReplacementPropagates) {
  auto instrumenting = boost::make_shared<InstrumentingBackend>("testSubArrayAccessorDecorator.map");
  auto target = instrumenting->getRegisterAccessor_impl<int32_t>(RegisterPath("/MY_ARRAY"), 0, 0, AccessModeFlags({}));

  auto deco = boost::make_shared<TestNDDecorator<int32_t>>(target);
  auto exceptionBackend = boost::make_shared<InstrumentingBackend>("testSubArrayAccessorDecorator.map");
  deco->setExceptionBackend(exceptionBackend);

  // an element which the target accepts as its replacement: a copy decorator ends up as the new target
  auto incoming = boost::dynamic_pointer_cast<CountingDecorator<int32_t>>(
      instrumenting->getRegisterAccessor_impl<int32_t>(RegisterPath("/MY_ARRAY"), 0, 0, AccessModeFlags({})));
  incoming->mayReplaceOthers = true;
  deco->replaceTransferElement(incoming);

  BOOST_TEST(deco->target()->getExceptionBackend() == exceptionBackend);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestForwardingPreservesExistingExceptionBackend) {
  // Regression: forwarding a replaceTransferElement() call must leave the chain's existing exception backend unchanged,
  // so backend exceptions continue to be reported to the same backend.
  auto instrumenting = boost::make_shared<InstrumentingBackend>("testSubArrayAccessorDecorator.map");
  auto target = instrumenting->getRegisterAccessor_impl<int32_t>(RegisterPath("/MY_ARRAY"), 0, 0, AccessModeFlags({}));

  auto deco = boost::make_shared<TestNDDecorator<int32_t>>(target);
  auto exceptionBackend = boost::make_shared<InstrumentingBackend>("testSubArrayAccessorDecorator.map");
  deco->setExceptionBackend(exceptionBackend);

  // a forwarded call (the incoming element cannot replace the target)
  auto otherBackend = boost::make_shared<InstrumentingBackend>("testSubArrayAccessorDecorator.map");
  auto other = otherBackend->getRegisterAccessor_impl<int32_t>(RegisterPath("/MY_ARRAY"), 0, 0, AccessModeFlags({}));
  deco->replaceTransferElement(other);

  BOOST_TEST(deco->getExceptionBackend() == exceptionBackend);
  BOOST_TEST(deco->target()->getExceptionBackend() == exceptionBackend);
}

/**********************************************************************************************************************/
/* SubArrayAccessorDecorator                                                                                          */
/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestSubArrayDecoratorForwardingDoesNotPropagate) {
  auto backend = boost::make_shared<InstrumentingBackend>("testSubArrayAccessorDecorator.map");
  auto deco = boost::make_shared<detail::SubArrayAccessorDecorator<int32_t, int32_t>>(
      backend, RegisterPath("/MY_ARRAY"), 3, 2, AccessModeFlags({}));

  // an element the target cannot replace -> forwarded without propagating our backend
  auto otherBackend = boost::make_shared<InstrumentingBackend>("testSubArrayAccessorDecorator.map");
  auto other = otherBackend->getRegisterAccessor_impl<int32_t>(RegisterPath("/MY_ARRAY"), 0, 0, AccessModeFlags({}));

  CountingDecorator<int32_t>::_counter = 0;
  deco->replaceTransferElement(other);
  BOOST_TEST(CountingDecorator<int32_t>::_counter == 0);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestSubArrayDecoratorReplacementPropagates) {
  auto backend = boost::make_shared<InstrumentingBackend>("testSubArrayAccessorDecorator.map");
  auto deco = boost::make_shared<detail::SubArrayAccessorDecorator<int32_t, int32_t>>(
      backend, RegisterPath("/MY_ARRAY"), 3, 2, AccessModeFlags({}));

  auto exceptionBackend = boost::make_shared<InstrumentingBackend>("testSubArrayAccessorDecorator.map");
  deco->setExceptionBackend(exceptionBackend);

  // an incoming accessor which the current target accepts as replacement; it is assigned directly, so it becomes the
  // new target (a standalone decorator which must not introduce a shared-accessor entry)
  auto replacement = boost::dynamic_pointer_cast<CountingDecorator<int32_t>>(
      backend->getRegisterAccessor_impl<int32_t>(RegisterPath("/MY_ARRAY"), 0, 0, AccessModeFlags({})));
  replacement->mayReplaceOthers = true;
  // the decorator combines the target's shared state with the incoming element's, so the incoming element must be
  // known to the backend's shared accessor bookkeeping
  backend->getSharedAccessors()->addTransferElement(replacement->getId());
  deco->replaceTransferElement(replacement);

  BOOST_TEST(replacement->getExceptionBackend() == exceptionBackend);
}

/**********************************************************************************************************************/
/* BitRangeAccessorDecorator                                                                                         */
/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestBitRangeDecoratorForwardingDoesNotPropagate) {
  auto backend = boost::make_shared<InstrumentingBackend>("testSubArrayAccessorDecorator.map");
  auto target = backend->getRegisterAccessor_impl<uint64_t>(RegisterPath("/MY_ARRAY"), 0, 0, AccessModeFlags({}));

  auto registerInfo = makeBitRangeRegisterInfo();
  auto deco = boost::make_shared<detail::BitRangeAccessorDecorator<uint8_t, false>>(
      backend, RegisterPath("/MY_ARRAY"), target, registerInfo, 1, 0);

  // an element the target cannot replace -> forwarded without propagating our backend
  auto otherBackend = boost::make_shared<InstrumentingBackend>("testSubArrayAccessorDecorator.map");
  auto other = otherBackend->getRegisterAccessor_impl<uint64_t>(RegisterPath("/MY_ARRAY"), 0, 0, AccessModeFlags({}));

  CountingDecorator<uint64_t>::_counter = 0;
  deco->replaceTransferElement(other);
  BOOST_TEST(CountingDecorator<uint64_t>::_counter == 0);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestBitRangeDecoratorReplacementPropagates) {
  auto backend = boost::make_shared<InstrumentingBackend>("testSubArrayAccessorDecorator.map");
  auto target = backend->getRegisterAccessor_impl<uint64_t>(RegisterPath("/MY_ARRAY"), 0, 0, AccessModeFlags({}));

  auto registerInfo = makeBitRangeRegisterInfo();
  auto deco = boost::make_shared<detail::BitRangeAccessorDecorator<uint8_t, false>>(
      backend, RegisterPath("/MY_ARRAY"), target, registerInfo, 1, 0);

  auto exceptionBackend = boost::make_shared<InstrumentingBackend>("testSubArrayAccessorDecorator.map");
  deco->setExceptionBackend(exceptionBackend);

  // the incoming element accepts the current target as replaceable, so the target is replaced directly
  auto replacement = boost::dynamic_pointer_cast<CountingDecorator<uint64_t>>(
      backend->getRegisterAccessor_impl<uint64_t>(RegisterPath("/MY_ARRAY"), 0, 0, AccessModeFlags({})));
  replacement->mayReplaceOthers = true;
  // the decorator combines the target's shared state with the incoming element's, so the incoming element must be
  // known to the backend's shared accessor bookkeeping
  backend->getSharedAccessors()->addTransferElement(replacement->getId());
  deco->replaceTransferElement(replacement);

  BOOST_TEST(deco->_target->getExceptionBackend() == exceptionBackend);
}

/**********************************************************************************************************************/
/* LNMBackendChannelAccessor                                                                                         */
/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestLNMChannelAccessorForwardingDoesNotPropagate) {
  auto dev = openLNMBackend();
  BOOST_REQUIRE(dev);

  auto deco =
      boost::make_shared<TestLNMChannelAccessor<int32_t>>(dev, RegisterPath("/Channel"), 0, 0, AccessModeFlags({}));

  // The target cannot be replaced by an arbitrary element (NDRegisterAccessorDecorator keeps the default
  // mayReplaceOther()), so the call is forwarded. The target must not receive our exception backend during the
  // forwarding call.
  auto otherBackend = boost::make_shared<InstrumentingBackend>("muxedDataAccessor.jmap");
  auto other = otherBackend->getRegisterAccessor_impl<int32_t>(RegisterPath("TEST.DMA"), 0, 0, AccessModeFlags({}));

  CountingDecorator<int32_t>::_counter = 0;
  deco->callReplace(other);
  BOOST_TEST(CountingDecorator<int32_t>::_counter == 0);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestLNMChannelAccessorReplacementPropagates) {
  auto dev = openLNMBackend();
  BOOST_REQUIRE(dev);

  auto deco =
      boost::make_shared<TestLNMChannelAccessor<int32_t>>(dev, RegisterPath("/Channel"), 0, 0, AccessModeFlags({}));

  auto exceptionBackend = boost::make_shared<InstrumentingBackend>("muxedDataAccessor.jmap");
  deco->setExceptionBackend(exceptionBackend);

  // the target accepts the incoming channel accessor as replaceable (same register path and device)
  auto target = boost::dynamic_pointer_cast<CountingDecorator<int32_t>>(deco->_accessor);
  BOOST_REQUIRE(target);
  target->mayReplaceOthers = true;
  auto replacement =
      boost::make_shared<TestLNMChannelAccessor<int32_t>>(dev, RegisterPath("/Channel"), 0, 0, AccessModeFlags({}));
  deco->callReplace(replacement);

  BOOST_TEST(deco->_accessor->getExceptionBackend() == exceptionBackend);
}

/**********************************************************************************************************************/
/* LNMBackendBitAccessor                                                                                             */
/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestLNMBitAccessorForwardingDoesNotPropagate) {
  auto dev = openLNMBackend();
  BOOST_REQUIRE(dev);

  auto deco =
      boost::make_shared<TestLNMBitAccessor<ChimeraTK::Boolean>>(dev, RegisterPath("/Bit"), 0, 0, AccessModeFlags({}));

  // an incoming bit accessor for a different register cannot replace the target -> forwarded without our backend
  auto otherBackend = boost::make_shared<InstrumentingBackend>("mtcadummy.map");
  auto other =
      otherBackend->getRegisterAccessor_impl<uint64_t>(RegisterPath("BOARD.WORD_USER"), 0, 0, AccessModeFlags({}));

  CountingDecorator<uint64_t>::_counter = 0;
  deco->callReplace(other);
  BOOST_TEST(CountingDecorator<uint64_t>::_counter == 0);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestLNMBitAccessorReplacementPropagates) {
  auto dev = openLNMBackend();
  BOOST_REQUIRE(dev);

  auto deco =
      boost::make_shared<TestLNMBitAccessor<ChimeraTK::Boolean>>(dev, RegisterPath("/Bit"), 0, 0, AccessModeFlags({}));

  auto exceptionBackend = boost::make_shared<InstrumentingBackend>("mtcadummy.map");
  deco->setExceptionBackend(exceptionBackend);

  // an incoming bit accessor for the same register and device can replace the target; the implementation assigns a
  // copy decorator around it, which must carry our exception backend
  auto replacement =
      boost::make_shared<TestLNMBitAccessor<ChimeraTK::Boolean>>(dev, RegisterPath("/Bit"), 0, 0, AccessModeFlags({}));
  deco->callReplace(replacement);

  BOOST_TEST(deco->_accessor->getExceptionBackend() == exceptionBackend);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
