// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ChimeraTK/ReadAnyGroup.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE DataConsistencyGroupTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "DataConsistencyDecorator.h"
#include "Device.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(ExtendedDataMatchingTestSuite)

BOOST_AUTO_TEST_CASE(test1) {
  // minimal test code: we create two read accessors on variables defined in xlmap, put them into a ReadAnyGroup and a
  // HDataConsistencyGroup. We provide data from another thread, as in real use case. Then using different delay
  // settings for updates of A and B, check expected number of consistent data updates.

  Device dev;
  dev.open("(logicalNameMap?map=extendedDataMatching.xlmap)");
  auto readAccA = dev.getScalarRegisterAccessor<int32_t>("/A", 0, {AccessMode::wait_for_new_data});
  auto readAccB = dev.getScalarRegisterAccessor<int32_t>("/B", 0, {AccessMode::wait_for_new_data});
  ChimeraTK::ReadAnyGroup rag{readAccA, readAccB};
  ChimeraTK::HDataConsistencyGroup dg{readAccA, readAccB};

  // for updater thread: option to delay updates on A and B
  // e.g. B has delay=2: A=v1, A=v2, A=v3, B=v1, A=v4, B=v2, ...
  auto updaterLoop = [&](unsigned nLoops, unsigned delay) {
    auto accA = dev.getScalarRegisterAccessor<int32_t>("/A");
    auto accB = dev.getScalarRegisterAccessor<int32_t>("/B");
    std::vector<ChimeraTK::VersionNumber> vs(nLoops);
    for(unsigned loopCount = 0; loopCount < nLoops; loopCount++) {
      accA = loopCount;
      accA.write(vs[loopCount]);
      if(loopCount >= delay) {
        accB = loopCount - delay;
        accB.write(vs[loopCount - delay]);
      }
    }
    // interrupt blocking reads in test thread so it can terminate
    readAccA.interrupt();
    readAccB.interrupt();
  };

  // TODOs
  // - without HDataConsistencyGroup, check that we never get consistent updates
  // - with HDataConsistencyGroup, check that we get N-delay consistent updates
  // - also check that we have consistent data (e.g. data=versionnumber counter)

  unsigned nConsistentUpdates = 0;
  std::thread updaterThread1(updaterLoop, 2, 0);
  // TODO test loop consuming data
  while(true) {
    auto id = rag.readAny();
    // bool isConsistent = dg.update(id);
    // check data consistency via VersionNumber
    BOOST_TEST(readAccA.getVersionNumber() == readAccB.getVersionNumber());
  }
  updaterThread1.join();
  BOOST_TEST(nConsistentUpdates == 2);
}

// TODO - how can we check things related to MetaDataPropagatingRegisterDecorator?
// maybe only in ApplicationCore, since that's were it's defined?

// Note, current test code testDataConsistencyGroup is totally unsuitable, it only
// tests based on changes in user-buffer.

BOOST_AUTO_TEST_SUITE_END()
