// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
// Define a name for the test module.
#define BOOST_TEST_MODULE SubArrayAccessorDecoratorTest
// Only after defining the name include the unit test header.
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "DummyBackend.h"
#include "SubArrayAccessorDecorator.h"
#include "TransferGroup.h"
using namespace ChimeraTK;

#include <iostream>

// This decorator does nothing, except swap from its own user buffer into the target user buffer and back.
// It implements a proper mayReplaceOther for this.
template<typename UserType>
class DoNothingDecorator : public NDRegisterAccessorDecorator<UserType, UserType> {
  using NDRegisterAccessorDecorator<UserType, UserType>::NDRegisterAccessorDecorator;

  // If the targets can replace each other, then the whole thing including the decorator can be replaced.
  [[nodiscard]] bool mayReplaceOther(const boost::shared_ptr<TransferElement const>& other) const override {
    auto casted = boost::dynamic_pointer_cast<DoNothingDecorator const>(other);
    if(!casted) {
      return false;
    }
    if(casted.get() == this) {
      return false;
    }
    return casted->_target->mayReplaceOther(this->_target);
  }
};

/// Test backend to put the SubArrayAccessorDecorator around the created accessors.
/// It also contains counters to track the number of read and write calls.
class SubArrayDecoratorTestBackend : public DummyBackend {
 public:
  DEFINE_VIRTUAL_FUNCTION_OVERRIDE_VTABLE(DummyBackendBase, getRegisterAccessor_impl,
      boost::shared_ptr<NDRegisterAccessor<T>>(const RegisterPath&, size_t, size_t, AccessModeFlags));

  explicit SubArrayDecoratorTestBackend(const std::string& mapFileName) : DummyBackend(mapFileName) {
    OVERRIDE_VIRTUAL_FUNCTION_TEMPLATE(DummyBackendBase, getRegisterAccessor_impl);
  }

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    auto registerInfo = getRegisterInfo(registerPathName);

    if((wordOffsetInRegister == 0) && ((numberOfWords == registerInfo.getNumberOfElements()) || numberOfWords == 0)) {
      // Put an empty decorator around the full accessor. This introduces an additional copy from the decorator's user
      // buffer to the target in pre-write (which otherwise is empty and it cannot be tested that it is called
      // correctly).
      auto fullAccessor = DummyBackend::getRegisterAccessor_impl<UserType>(
          registerPathName, registerInfo.getNumberOfElements(), 0 /* offset */, flags);
      return boost::make_shared<DoNothingDecorator<UserType>>(fullAccessor);
    }

    return boost::make_shared<ChimeraTK::detail::SubArrayAccessorDecorator<UserType, UserType>>(
        shared_from_this(), registerPathName, numberOfWords, wordOffsetInRegister, flags);
  }

  size_t writeCount{0};
  size_t readCount{0};

  void read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) override {
    ++readCount;
    DummyBackend::read(bar, address, data, sizeInBytes);
  }

  void write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) override {
    ++writeCount;
    DummyBackend::write(bar, address, data, sizeInBytes);
  }

  static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string> parameters) {
    return boost::make_shared<SubArrayDecoratorTestBackend>(convertPathRelativeToDmapToAbs(parameters.at("map")));
  }

 private:
  struct Registerer {
    Registerer() {
      ChimeraTK::BackendFactory::getInstance().registerBackendType(
          "SubArrayDecoratorTestBackend", &SubArrayDecoratorTestBackend::createInstance, {"map"});
    }
  };
  static Registerer _registerer;
};

SubArrayDecoratorTestBackend::Registerer SubArrayDecoratorTestBackend::_registerer;

struct TheFixture {
  ChimeraTK::Device device{"(SubArrayDecoratorTestBackend?map=testSubArrayAccessorDecorator.map)"};
  OneDRegisterAccessor<int32_t> fullAccessor;

  TheFixture() {
    device.open();
    fullAccessor.replace(device.getOneDRegisterAccessor<int32_t>("/MY_ARRAY"));
    fullAccessor = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
    fullAccessor.write();
  }
};

BOOST_FIXTURE_TEST_CASE(TestSize, TheFixture) {
  auto accessor234 = device.getOneDRegisterAccessor<int32_t>("/MY_ARRAY", 3, 2);
  BOOST_TEST(accessor234.getNElements() == 3);
}

BOOST_FIXTURE_TEST_CASE(TestRead, TheFixture) {
  auto accessor234 = device.getOneDRegisterAccessor<int32_t>("/MY_ARRAY", 3, 2);
  accessor234.read();
  BOOST_TEST(accessor234[0] == 12);
  BOOST_TEST(accessor234[1] == 13);
  BOOST_TEST(accessor234[2] == 14);
}

BOOST_FIXTURE_TEST_CASE(TestReadOnceRememberModifyWrite, TheFixture) {
  // Step one: Read
  // The first write is reading to update the accessor's internal state.
  auto accessor234 = device.getOneDRegisterAccessor<int32_t>("/MY_ARRAY", 3, 2);
  accessor234 = {22, 23, 24};
  accessor234.write();

  // TEST 1: The previous content stays
  fullAccessor.read();
  BOOST_TEST(fullAccessor[0] == 10);
  BOOST_TEST(fullAccessor[1] == 11);
  BOOST_TEST(fullAccessor[2] == 22);
  BOOST_TEST(fullAccessor[3] == 23);
  BOOST_TEST(fullAccessor[4] == 24);
  BOOST_TEST(fullAccessor[5] == 15);
  BOOST_TEST(fullAccessor[6] == 16);
  BOOST_TEST(fullAccessor[7] == 17);
  BOOST_TEST(fullAccessor[8] == 18);
  BOOST_TEST(fullAccessor[9] == 19);

  // Step two: Use the remembered value and don't read

  // Change the content on the device for this test. This will not be seen by the accessor and overwritten. It is not a
  // valid wanted scenario but used in this test to check that the performance optimisation of reading only once actually works.
  fullAccessor[9] = 29;
  fullAccessor.write();
  // Also monitor the read counter. We are observing a performance improvement here.
  auto dummy = boost::dynamic_pointer_cast<SubArrayDecoratorTestBackend>(device.getBackend());
  auto oldReadCount = dummy->readCount;

  accessor234 = {32, 33, 34};
  accessor234.write();

  // TEST 2: No read. New partial content and remembered content are written.
  BOOST_TEST(oldReadCount == dummy->readCount);
  fullAccessor.read();
  BOOST_TEST(fullAccessor[0] == 10);
  BOOST_TEST(fullAccessor[1] == 11);
  BOOST_TEST(fullAccessor[2] == 32); // changed
  BOOST_TEST(fullAccessor[3] == 33); // changed
  BOOST_TEST(fullAccessor[4] == 34); // changed
  BOOST_TEST(fullAccessor[5] == 15);
  BOOST_TEST(fullAccessor[6] == 16);
  BOOST_TEST(fullAccessor[7] == 17);
  BOOST_TEST(fullAccessor[8] == 18);
  BOOST_TEST(fullAccessor[9] == 19); // changed

  // Step 3: After an exception, the read is performed again to get changes on the device.
  // Change the content on the device.
  fullAccessor[9] = 49;
  fullAccessor.write();

  device.setException("exception just for test");
  device.open();

  accessor234 = {42, 43, 44};
  accessor234.write();

  // TEST 3: The read before write is performed to pick up the changes on the device.
  fullAccessor.read();
  BOOST_TEST(fullAccessor[0] == 10);
  BOOST_TEST(fullAccessor[1] == 11);
  BOOST_TEST(fullAccessor[2] == 42); // changed
  BOOST_TEST(fullAccessor[3] == 43); // changed
  BOOST_TEST(fullAccessor[4] == 44); // changed
  BOOST_TEST(fullAccessor[5] == 15);
  BOOST_TEST(fullAccessor[6] == 16);
  BOOST_TEST(fullAccessor[7] == 17);
  BOOST_TEST(fullAccessor[8] == 18);
  BOOST_TEST(fullAccessor[9] == 49); // not changed
}

BOOST_FIXTURE_TEST_CASE(TestRememberModifyWrite, TheFixture) {
  auto accessor1 = device.getScalarRegisterAccessor<int32_t>("/MY_ARRAY_WO", 1);
  auto accessor234 = device.getOneDRegisterAccessor<int32_t>("/MY_ARRAY_WO", 3, 2);
  accessor234 = {22, 23, 24};
  accessor234.write();

  fullAccessor.read();
  BOOST_TEST(fullAccessor[0] == 0);
  BOOST_TEST(fullAccessor[1] == 0);
  BOOST_TEST(fullAccessor[2] == 22);
  BOOST_TEST(fullAccessor[3] == 23);
  BOOST_TEST(fullAccessor[4] == 24);
  BOOST_TEST(fullAccessor[5] == 0);
  BOOST_TEST(fullAccessor[6] == 0);
  BOOST_TEST(fullAccessor[7] == 0);
  BOOST_TEST(fullAccessor[8] == 0);
  BOOST_TEST(fullAccessor[9] == 0);

  accessor1 = 21;
  accessor1.write();
  fullAccessor.read();
  BOOST_TEST(fullAccessor[0] == 0);
  BOOST_TEST(fullAccessor[1] == 21);
  BOOST_TEST(fullAccessor[2] == 22); // This is the actual test: value is remembered, not overwritten!
  BOOST_TEST(fullAccessor[3] == 23); // This is the actual test: value is remembered, not overwritten!
  BOOST_TEST(fullAccessor[4] == 24); // This is the actual test: value is remembered, not overwritten!
  BOOST_TEST(fullAccessor[5] == 0);
  BOOST_TEST(fullAccessor[6] == 0);
  BOOST_TEST(fullAccessor[7] == 0);
  BOOST_TEST(fullAccessor[8] == 0);
  BOOST_TEST(fullAccessor[9] == 0);
}

/**
 * Read is working in a transfer group.
 */
BOOST_FIXTURE_TEST_CASE(TestTransferGroupRead, TheFixture) {
  auto acc1 = device.getScalarRegisterAccessor<int32_t>("/MY_ARRAY", 1);
  auto acc234 = device.getOneDRegisterAccessor<int32_t>("/MY_ARRAY", 3, 2);

  acc1.read();   // initial value
  acc234.read(); // initial value

  ChimeraTK::TransferGroup tg;
  tg.addAccessor(acc1);
  tg.addAccessor(acc234);

  fullAccessor = {30, 31, 32, 33, 34, 35, 36, 37, 38, 39};
  fullAccessor.write();

  tg.read();

  BOOST_TEST(acc1 == 31.);
  BOOST_TEST(acc234[0] == 32.);
  BOOST_TEST(acc234[1] == 33.);
  BOOST_TEST(acc234[2] == 34.);
}

/**
 * Read is merging the transfer: Only one read.
 */
BOOST_FIXTURE_TEST_CASE(TestReadMerging, TheFixture) {
  auto acc1 = device.getScalarRegisterAccessor<int32_t>("/MY_ARRAY", 1);
  auto acc234 = device.getOneDRegisterAccessor<int32_t>("/MY_ARRAY", 3, 2);

  acc1.read();   // initial value
  acc234.read(); // initial value

  ChimeraTK::TransferGroup tg;
  tg.addAccessor(acc1);
  tg.addAccessor(acc234);

  auto dummy = boost::dynamic_pointer_cast<SubArrayDecoratorTestBackend>(device.getBackend());
  assert(dummy);

  auto oldReadCount = dummy->readCount;

  tg.read();

  BOOST_TEST(dummy->readCount == oldReadCount + 1);
}

/**
 * Read-Modify-Write is working in a transfer group.
 */
BOOST_FIXTURE_TEST_CASE(TestTransferGroupReadModifyWrite, TheFixture) {
  auto acc1 = device.getScalarRegisterAccessor<int32_t>("/MY_ARRAY", 1);
  auto acc234 = device.getOneDRegisterAccessor<int32_t>("/MY_ARRAY", 3, 2);

  ChimeraTK::TransferGroup tg;
  tg.addAccessor(acc1);
  tg.addAccessor(acc234);

  acc1 = 41;
  acc234 = {42, 43, 44};

  tg.write();

  fullAccessor.read();

  BOOST_TEST(fullAccessor[0] == 10); // not modified
  BOOST_TEST(fullAccessor[1] == 41); // acc1
  BOOST_TEST(fullAccessor[2] == 42); // acc234[0]
  BOOST_TEST(fullAccessor[3] == 43); // acc234[1]
  BOOST_TEST(fullAccessor[4] == 44); // acc234[2]
  BOOST_TEST(fullAccessor[5] == 15); // not modified
  BOOST_TEST(fullAccessor[6] == 16); // not modified
  BOOST_TEST(fullAccessor[7] == 17); // not modified
  BOOST_TEST(fullAccessor[8] == 18); // not modified
  BOOST_TEST(fullAccessor[9] == 19); // not modified
}

/**
 * Transfer merging: One read and one write on the backend.
 */
BOOST_FIXTURE_TEST_CASE(TestReadModifyWriteMerging, TheFixture) {
  auto acc1 = device.getScalarRegisterAccessor<int32_t>("/MY_ARRAY", 1);
  auto acc234 = device.getOneDRegisterAccessor<int32_t>("/MY_ARRAY", 3, 2);

  ChimeraTK::TransferGroup tg;
  tg.addAccessor(acc1);
  tg.addAccessor(acc234);

  auto dummy = boost::dynamic_pointer_cast<SubArrayDecoratorTestBackend>(device.getBackend());
  assert(dummy);

  auto oldReadCount = dummy->readCount;
  auto oldWriteCount = dummy->writeCount;

  tg.write();
  BOOST_TEST(dummy->readCount == oldReadCount + 1);
  BOOST_TEST(dummy->writeCount == oldWriteCount + 1);
}

/*****/
/* Write-Only is working in a transfer group. */
/****/
BOOST_FIXTURE_TEST_CASE(TestTransferGroupWriteOnly, TheFixture) {
  auto acc1 = device.getScalarRegisterAccessor<int32_t>("/MY_ARRAY_WO", 1);
  auto acc234 = device.getOneDRegisterAccessor<int32_t>("/MY_ARRAY_WO", 3, 2);

  ChimeraTK::TransferGroup tg;
  tg.addAccessor(acc1);
  tg.addAccessor(acc234);

  acc1 = 41;
  acc234 = {42, 43, 44};

  tg.write();

  fullAccessor.read();

  BOOST_TEST(fullAccessor[0] == 0);  // From inital target buffer
  BOOST_TEST(fullAccessor[1] == 41); // acc1
  BOOST_TEST(fullAccessor[2] == 42); // acc234[0]
  BOOST_TEST(fullAccessor[3] == 43); // acc234[1]
  BOOST_TEST(fullAccessor[4] == 44); // acc234[2]
  BOOST_TEST(fullAccessor[5] == 0);  // From inital target buffer
  BOOST_TEST(fullAccessor[6] == 0);  // From inital target buffer
  BOOST_TEST(fullAccessor[7] == 0);  // From inital target buffer
  BOOST_TEST(fullAccessor[8] == 0);  // From inital target buffer
  BOOST_TEST(fullAccessor[9] == 0);  // From inital target buffer
}

/*****/
/* Transfer merging for Write-Only: One write, not reads. */
/****/
BOOST_FIXTURE_TEST_CASE(TestTransferGroupWriteOnlyMerging, TheFixture) {
  auto acc1 = device.getScalarRegisterAccessor<int32_t>("/MY_ARRAY_WO", 1);
  auto acc234 = device.getOneDRegisterAccessor<int32_t>("/MY_ARRAY_WO", 3, 2);

  ChimeraTK::TransferGroup tg;
  tg.addAccessor(acc1);
  tg.addAccessor(acc234);

  auto dummy = boost::dynamic_pointer_cast<SubArrayDecoratorTestBackend>(device.getBackend());
  assert(dummy);

  auto oldReadCount = dummy->readCount;
  auto oldWriteCount = dummy->writeCount;

  tg.write();

  BOOST_TEST(dummy->readCount == oldReadCount);
  BOOST_TEST(dummy->writeCount == oldWriteCount + 1);
}

/*****/
/* Overlapping sub arrays turn the accessors to read-only mode. */
/****/
BOOST_FIXTURE_TEST_CASE(TestOverlapping, TheFixture) {
  auto acc12 = device.getOneDRegisterAccessor<int32_t>("/MY_ARRAY", 2, 1);
  auto acc234 = device.getOneDRegisterAccessor<int32_t>("/MY_ARRAY", 3, 2);

  ChimeraTK::TransferGroup tg;
  tg.addAccessor(acc12);
  tg.addAccessor(acc234);

  BOOST_TEST(acc12.isReadOnly());
  BOOST_TEST(acc234.isReadOnly());
  BOOST_TEST(!acc12.isWriteable());
  BOOST_TEST(!acc234.isWriteable());
}
