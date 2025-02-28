// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE DataConsistencyGroupTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "DataConsistencyGroup.h"
#include "Device.h"
#include "NDRegisterAccessor.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(DataConsistencyGroupTestSuite)

// notice: you cannot read from this accessor. It will block forever because there is nobody to write into the
// _readQueue.
template<typename UserType>
class Accessor : public NDRegisterAccessor<UserType> {
 public:
  Accessor() : NDRegisterAccessor<UserType>("", {AccessMode::wait_for_new_data}) {}

  ~Accessor() override {}

  void doReadTransferSynchronously() override {}

  bool doWriteTransfer(ChimeraTK::VersionNumber) override { return true; }

  void doPreWrite(TransferType, VersionNumber) override {}

  void doPostWrite(TransferType, VersionNumber) override {}

  void doPreRead(TransferType) override {}

  void doPostRead(TransferType, bool /*hasNewData*/) override {}

  // AccessModeFlags getAccessModeFlags() const override { return {AccessMode::wait_for_new_data}; }
  bool isReadOnly() const override { return false; }
  bool isReadable() const override { return true; }
  bool isWriteable() const override { return true; }

  std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
    return {this->shared_from_this()};
  }
  std::list<boost::shared_ptr<TransferElement>> getInternalElements() override { return {}; }

  //  VersionNumber getVersionNumber() const override { return currentVersion; }

  //  VersionNumber currentVersion;
};

BOOST_AUTO_TEST_CASE(testDataConsistencyGroup) {
  boost::shared_ptr<Accessor<int>> acc_1 = boost::make_shared<Accessor<int>>();
  boost::shared_ptr<Accessor<int>> acc_2 = boost::make_shared<Accessor<int>>();

  // test the deprecated syntax without warning about it...
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  DataConsistencyGroup dcgroup({acc_1, acc_2});
#pragma GCC diagnostic pop

  // until now all versions are {nullptr}
  //  prepare the version numbers in the dcgroup by writing (which set new version numbers)
  acc_1->write();
  acc_2->write();

  BOOST_CHECK(dcgroup.update(acc_1->getId()) == false);

  BOOST_CHECK(dcgroup.update(acc_2->getId()) == false);

  // now update acc_1 with the newer version number from acc_2
  acc_1->write(acc_2->getVersionNumber());

  BOOST_CHECK(dcgroup.update(acc_1->getId()) == true);
  BOOST_CHECK(dcgroup.update(acc_1->getId()) == true);
  BOOST_CHECK(dcgroup.update(acc_2->getId()) == true);
  BOOST_CHECK(dcgroup.update(acc_2->getId()) == true);
  BOOST_CHECK(dcgroup.update(acc_2->getId()) == true);
}

BOOST_AUTO_TEST_CASE(testMoreDataConsistencyGroup) {
  boost::shared_ptr<Accessor<int>> acc_1 = boost::make_shared<Accessor<int>>();
  boost::shared_ptr<Accessor<int>> acc_2 = boost::make_shared<Accessor<int>>();
  boost::shared_ptr<Accessor<int>> acc_3 = boost::make_shared<Accessor<int>>();
  boost::shared_ptr<Accessor<int>> acc_4 = boost::make_shared<Accessor<int>>();

  // test the deprecated syntax without warning about it...
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  DataConsistencyGroup dcgroup({acc_1, acc_2, acc_3, acc_4});
#pragma GCC diagnostic pop
  // 4 different version numbers
  acc_1->write();
  acc_2->write();
  acc_3->write();
  acc_4->write();
  BOOST_CHECK(dcgroup.update(acc_1->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_2->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_3->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_4->getId()) == false);

  // 3 different version numbers, acc_1 and acc_2 are the same
  VersionNumber v; // a new version number
  acc_1->write(v);
  acc_2->write(v);
  BOOST_CHECK(dcgroup.update(acc_1->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_2->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_3->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_4->getId()) == false);

  acc_3->write(v);
  BOOST_CHECK(dcgroup.update(acc_1->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_2->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_3->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_4->getId()) == false);

  acc_4->write(v);
  BOOST_CHECK(dcgroup.update(acc_1->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_2->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_3->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_4->getId()) == true);
  BOOST_CHECK(dcgroup.update(acc_2->getId()) == true);
  BOOST_CHECK(dcgroup.update(acc_4->getId()) == true);
  BOOST_CHECK(dcgroup.update(acc_3->getId()) == true);
  BOOST_CHECK(dcgroup.update(acc_1->getId()) == true);

  // push an accessor that does not belong to DataConsistencyGroup, should be ignored, although it hat the same version
  // number
  boost::shared_ptr<Accessor<int>> acc_5 = boost::make_shared<Accessor<int>>();
  acc_5->write(v);
  BOOST_CHECK(dcgroup.update(acc_5->getId()) == false);
}

// The same TransferElement shall be allowed to be part of multiple DataConsistencyGroups at the same time.
BOOST_AUTO_TEST_CASE(testMultipleDataConsistencyGroup) {
  boost::shared_ptr<Accessor<int>> acc_1 = boost::make_shared<Accessor<int>>();
  boost::shared_ptr<Accessor<int>> acc_2 = boost::make_shared<Accessor<int>>();
  boost::shared_ptr<Accessor<int>> acc_3 = boost::make_shared<Accessor<int>>();
  boost::shared_ptr<Accessor<int>> acc_4 = boost::make_shared<Accessor<int>>();
  // test the deprecated syntax without warning about it...
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  DataConsistencyGroup dcgroup_1({acc_1, acc_2, acc_3});
  DataConsistencyGroup dcgroup_2({acc_1, acc_3, acc_4});
#pragma GCC diagnostic pop
  VersionNumber v;
  acc_1->write(v);
  acc_2->write(v);
  acc_3->write(v);
  acc_4->write(v);
  BOOST_CHECK(dcgroup_1.update(acc_1->getId()) == false);
  BOOST_CHECK(dcgroup_1.update(acc_2->getId()) == false);
  BOOST_CHECK(dcgroup_1.update(acc_3->getId()) == true);
  BOOST_CHECK(dcgroup_1.update(acc_4->getId()) == false); // ignored
  BOOST_CHECK(dcgroup_2.update(acc_1->getId()) == false);
  BOOST_CHECK(dcgroup_2.update(acc_3->getId()) == false);
  BOOST_CHECK(dcgroup_2.update(acc_4->getId()) == true);
  BOOST_CHECK(dcgroup_2.update(acc_2->getId()) == false); // ignored
}

BOOST_AUTO_TEST_CASE(testVersionNumberChange) {
  VersionNumber v1{};
  VersionNumber v2{};
  VersionNumber v3{};

  boost::shared_ptr<Accessor<int>> acc_1 = boost::make_shared<Accessor<int>>();
  boost::shared_ptr<Accessor<int>> acc_2 = boost::make_shared<Accessor<int>>();

  // test the deprecated syntax without warning about it...
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  DataConsistencyGroup dcgroup_1({acc_1, acc_2});
#pragma GCC diagnostic pop

  acc_2->write(v2);
  BOOST_CHECK_EQUAL(dcgroup_1.update(acc_2->getId()), false);

  acc_1->write(v1);
  BOOST_CHECK_EQUAL(dcgroup_1.update(acc_1->getId()), false);

  acc_1->write(v2);
  BOOST_CHECK_EQUAL(dcgroup_1.update(acc_1->getId()), true);

  acc_1->write(v3);
  acc_2->write(v3);
  BOOST_CHECK_EQUAL(dcgroup_1.update(acc_1->getId()), false);
  BOOST_CHECK_EQUAL(dcgroup_1.update(acc_2->getId()), true);
}

BOOST_AUTO_TEST_CASE(testException) {
  Device dev;
  dev.open("(dummy?map=registerAccess.map)");
  auto acc = dev.getScalarRegisterAccessor<int>("BOARD.WORD_FIRMWARE");
  DataConsistencyGroup dcgroup;
  // accessors without wait_for_new_data cannot be added.
  // test the deprecated syntax without warning about it...
  BOOST_CHECK_THROW(dcgroup.add(acc), ChimeraTK::logic_error);
}

BOOST_AUTO_TEST_SUITE_END()
