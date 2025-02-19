// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <thread>
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE DataConsistencyGroupTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "DataConsistencyDecorator.h"
#include "Device.h"
#include "ReadAnyGroup.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(ExtendedDataMatchingTestSuite)
// Note, test code testDataConsistencyGroup is unsuitable for testing extended data matching,
// since it is based on explicitly provided changes in user-buffers.

BOOST_AUTO_TEST_CASE(test1) {
  // minimal test code: we create two read accessors on variables defined in xlmap, put them into a ReadAnyGroup and a
  // HDataConsistencyGroup. We provide data from another thread, as in real use case. Then using different delay
  // settings for updates of A and B, check expected number of consistent data updates.

  Device dev;
  dev.open("(logicalNameMap?map=extendedDataMatching.xlmap)");
  dev.activateAsyncRead();
  auto readAccA = dev.getScalarRegisterAccessor<int32_t>("/A", 0, {AccessMode::wait_for_new_data});
  auto readAccB = dev.getScalarRegisterAccessor<int32_t>("/B", 0, {AccessMode::wait_for_new_data});
  ChimeraTK::ReadAnyGroup rag{readAccA, readAccB};

  auto emptyQueue = [&rag]() {
    // empty read queue before start of next test.
    // this also gets rid of initial values
    while(rag.readAnyNonBlocking().isValid()) {
    }
  };

  // we use a semaphore to acknowlegde data received; like that we avoid future_queue overruns
  sem_t sem;
  sem_init(&sem, 0, 0);
  auto waitOnReceiveAck = [&]() {
    // we need a timeout since with HDataConsistencyGroup, we only get a chance to acknowledge consistent data updates
    timespec timeout{};
    timeval now{};
    gettimeofday(&now, nullptr);
    // Set timeout 1 seconds from now
    timeout.tv_sec = now.tv_sec + 1;
    timeout.tv_nsec = now.tv_usec * 1000;
    sem_timedwait(&sem, &timeout);
  };

  // loop for updater thread: option to delay updates on A and B
  // e.g. B has delay=2: A=v1, A=v2, A=v3, B=v1, A=v4, B=v2, ...
  auto updaterLoop = [&](unsigned nLoops, unsigned delay) {
    auto accA = dev.getScalarRegisterAccessor<int32_t>("/A");
    auto accB = dev.getScalarRegisterAccessor<int32_t>("/B");
    std::vector<ChimeraTK::VersionNumber> vs(nLoops);
    for(unsigned loopCount = 0; loopCount < nLoops; loopCount++) {
      accA = (int32_t)loopCount;
      accA.write(vs[loopCount]);
      if(loopCount >= delay) {
        accB = (int32_t)(loopCount - delay);
        accB.write(vs[loopCount - delay]);
      }
      // wait on data receive before writing next
      waitOnReceiveAck();
    }
    // interrupt blocking reads in test thread so it can terminate. It should be sufficient to
    // interrupt one of the read accessors.
    readAccA.interrupt();
  };

  const unsigned nLoops = 4;
  for(unsigned delay = 0; delay <= 2; delay++) {
    // without HDataConsistencyGroup, check that we get consistent updates only if delay is 0
    emptyQueue();
    unsigned nUpdates = 0;
    unsigned nConsistentUpdates = 0;
    std::thread updaterThread1(updaterLoop, nLoops, delay);
    // test loop consuming data
    try {
      while(true) {
        auto id = rag.readAny();
        auto& acc = id == readAccA.getId() ? readAccA : readAccB;
        std::cout << "readAny: seeing update for target " << acc.getName() << " vs " << acc.getVersionNumber()
                  << std::endl;

        nUpdates++;
        if(readAccA.getVersionNumber() == readAccB.getVersionNumber()) {
          nConsistentUpdates++;
        }
        if(id == readAccA.getId()) {
          sem_post(&sem);
        }
      }
    }
    catch(boost::thread_interrupted& e) {
      std::cout << "thread interrupted" << std::endl;
    }
    updaterThread1.join();

    unsigned nExpectedConsistentUpdates = (delay == 0) ? nUpdates / 2 : 0;
    BOOST_TEST(nConsistentUpdates == nExpectedConsistentUpdates);
    BOOST_TEST(nUpdates == nLoops + nLoops - delay);
  }

  ChimeraTK::HDataConsistencyGroup dg{readAccA, readAccB};
  // TODO discuss API:
  // ReadAnyGroup holds copy of acessors(abstractors), so these also need to become decorated.
  dg.decorateAccessors(&rag);

  for(unsigned delay = 0; delay <= 2; delay++) {
    // With HDataConsistencyGroup, check that we get N-delay consistent updates.
    // Also check that we have consistent data (e.g. data=versionnumber counter)
    emptyQueue();
    unsigned nUpdates = 0;
    unsigned nConsistentUpdates = 0;
    std::thread updaterThread1(updaterLoop, nLoops, delay);
    // test loop consuming data
    try {
      while(true) {
        auto id = rag.readAny();
        // bool isConsistent = dg.update(id);
        nUpdates++;
        if(readAccA.getVersionNumber() == readAccB.getVersionNumber()) {
          nConsistentUpdates++;
        }
        // check data consistency via VersionNumber and content
        BOOST_TEST(readAccA.getVersionNumber() == readAccB.getVersionNumber());
        BOOST_TEST(readAccA == readAccB);
        // acknowledge data received: here updated id is irrelevant
        sem_post(&sem);
      }
    }
    catch(boost::thread_interrupted& e) {
      std::cout << "thread interrupted" << std::endl;
    }
    updaterThread1.join();
    BOOST_TEST(nConsistentUpdates == nLoops - delay);
    BOOST_TEST(nConsistentUpdates == nUpdates);
  }
}

// TODO - we should extend test, for case exceptions are thrown from backend:
// use ExceptionDummy & interrupts

// TODO - how can we check things related to MetaDataPropagatingRegisterDecorator?
// maybe only in ApplicationCore, since that's were it's defined?

BOOST_AUTO_TEST_SUITE_END()
