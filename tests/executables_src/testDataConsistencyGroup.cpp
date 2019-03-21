#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE DataConsistencyGroupTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "DataConsistencyGroup.h"
#include "NDRegisterAccessor.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(DataConsistencyGroupTestSuite)

template<typename UserType>
class Accessor : public NDRegisterAccessor<UserType> {
 public:
  Accessor() : NDRegisterAccessor<UserType>("") {}

  ~Accessor() override {}

  TransferFuture doReadTransferAsync() override { return {}; }

  void doReadTransfer() override { doReadTransferAsync().wait(); }

  bool doReadTransferNonBlocking() override { return true; }

  bool doReadTransferLatest() override { return true; }

  bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber = {}) override {
    currentVersion = versionNumber;
    return true;
  }

  void doPreWrite() override {}

  void doPostWrite() override {}

  void doPreRead() override {}

  void doPostRead() override {}

  AccessModeFlags getAccessModeFlags() const override { return {AccessMode::wait_for_new_data}; }
  bool isReadOnly() const override { return false; }
  bool isReadable() const override { return true; }
  bool isWriteable() const override { return true; }

  std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
    return {this->shared_from_this()};
  }
  std::list<boost::shared_ptr<TransferElement>> getInternalElements() override { return {}; }

  VersionNumber getVersionNumber() const override { return currentVersion; }

  VersionNumber currentVersion;
};

BOOST_AUTO_TEST_CASE(testDataConsistencyGroup) {
  boost::shared_ptr<Accessor<int>> acc_1 = boost::make_shared<Accessor<int>>();
  boost::shared_ptr<Accessor<int>> acc_2 = boost::make_shared<Accessor<int>>();

  DataConsistencyGroup dcgroup({acc_1, acc_2});

  BOOST_CHECK(dcgroup.update(acc_1->getId()) == false);

  BOOST_CHECK(dcgroup.update(acc_2->getId()) == false);

  acc_1->currentVersion = acc_2->currentVersion;

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

  DataConsistencyGroup dcgroup({acc_1, acc_2, acc_3, acc_4});
  // 4 different version numbers
  BOOST_CHECK(dcgroup.update(acc_1->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_2->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_3->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_4->getId()) == false);

  // 3 different version numbers, acc_1 and acc_2 are the same
  acc_1->currentVersion = acc_2->currentVersion;
  BOOST_CHECK(dcgroup.update(acc_1->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_2->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_3->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_4->getId()) == false);

  acc_1->currentVersion = acc_2->currentVersion;
  acc_3->currentVersion = acc_2->currentVersion;
  BOOST_CHECK(dcgroup.update(acc_1->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_2->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_3->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_4->getId()) == false);

  acc_1->currentVersion = acc_2->currentVersion;
  acc_3->currentVersion = acc_2->currentVersion;
  acc_4->currentVersion = acc_2->currentVersion;
  BOOST_CHECK(dcgroup.update(acc_1->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_2->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_3->getId()) == false);
  BOOST_CHECK(dcgroup.update(acc_4->getId()) == true);
  BOOST_CHECK(dcgroup.update(acc_2->getId()) == true);
  BOOST_CHECK(dcgroup.update(acc_4->getId()) == true);
  BOOST_CHECK(dcgroup.update(acc_3->getId()) == true);
  BOOST_CHECK(dcgroup.update(acc_1->getId()) == true);

  // push a version number into update, that does not belong to DataConsistencyGroup, should be ignored
  boost::shared_ptr<Accessor<int>> acc_5 = boost::make_shared<Accessor<int>>();
  BOOST_CHECK(dcgroup.update(acc_5->getId()) == false);
}

//The same TransferElement shall be allowed to be part of multiple DataConsistencyGroups at the same time.
BOOST_AUTO_TEST_CASE(testMultipleDataConsistencyGroup) {
  boost::shared_ptr<Accessor<int>> acc_1 = boost::make_shared<Accessor<int>>();
  boost::shared_ptr<Accessor<int>> acc_2 = boost::make_shared<Accessor<int>>();
  boost::shared_ptr<Accessor<int>> acc_3 = boost::make_shared<Accessor<int>>();
  boost::shared_ptr<Accessor<int>> acc_4 = boost::make_shared<Accessor<int>>();
  DataConsistencyGroup dcgroup_1({acc_1, acc_2, acc_3});
  DataConsistencyGroup dcgroup_2({acc_1, acc_3, acc_4});
  acc_1->currentVersion = acc_2->currentVersion;
  acc_3->currentVersion = acc_2->currentVersion;
  acc_4->currentVersion = acc_2->currentVersion;
  BOOST_CHECK(dcgroup_1.update(acc_1->getId()) == false);
  BOOST_CHECK(dcgroup_1.update(acc_2->getId()) == false);
  BOOST_CHECK(dcgroup_1.update(acc_3->getId()) == true);
  BOOST_CHECK(dcgroup_1.update(acc_4->getId()) == false); //ignored
  BOOST_CHECK(dcgroup_2.update(acc_1->getId()) == false);
  BOOST_CHECK(dcgroup_2.update(acc_2->getId()) == false); //ignored
  BOOST_CHECK(dcgroup_2.update(acc_3->getId()) == false);
  BOOST_CHECK(dcgroup_2.update(acc_4->getId()) == true);
}

BOOST_AUTO_TEST_CASE(testException) {
  Device dev;
  dev.open("(dummy?map=registerAccess.map)");
  auto acc = dev.getScalarRegisterAccessor<int>("BOARD.WORD_FIRMWARE");
  DataConsistencyGroup dcgroup;
  // accessors without wait_for_new_data cannot be added.
  BOOST_CHECK_THROW(dcgroup.add(acc), ChimeraTK::logic_error);
}

BOOST_AUTO_TEST_SUITE_END()
