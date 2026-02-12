// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <thread>
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE DataConsistencyGroupTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "DataConsistencyGroup.h"
#include "Device.h"
#include "NDRegisterAccessorDecorator.h"
#include "ReadAnyGroup.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(HistorizedDataMatchingTestSuite)
// Note, test code testDataConsistencyGroup is unsuitable for testing extended data matching,
// since it is based on explicitly provided changes in user-buffers.

auto emptyQueues(ReadAnyGroup& rag, DataConsistencyGroup* dg = nullptr) {
  // empty read queue before start of next test.
  // this also gets rid of initial values
  unsigned numDiscarded = 0;
  TransferElementID id;
  while((id = rag.readAnyNonBlocking()).isValid()) {
    if(dg) {
      dg->update(id);
    }
    numDiscarded++;
  }
  return numDiscarded;
};

struct Fixture {
  Fixture() {
    sem_init(&sem, 0, 0);

    dev.open("(logicalNameMap?map=historizedDataMatching.xlmap)");
    dev.activateAsyncRead();
    readAccA.replace(dev.getScalarRegisterAccessor<int32_t>("/A", 0, {AccessMode::wait_for_new_data}));
    readAccB.replace(dev.getScalarRegisterAccessor<int32_t>("/B", 0, {AccessMode::wait_for_new_data}));
  }

  void waitOnReceiveAck() {
    // we need a timeout since with HDataConsistencyGroup, we only get a chance to acknowledge consistent data updates
    timespec timeout{};
    timeval now{};
    gettimeofday(&now, nullptr);
    timeout.tv_sec = now.tv_sec + 1;
    timeout.tv_nsec = now.tv_usec * 300;
    sem_timedwait(&sem, &timeout);
  };

  // loop for updater thread: option to delay updates on A and B
  // e.g. B has delay=2: A=v1, A=v2, A=v3, B=v1, A=v4, B=v2, ...
  void updaterLoop(unsigned nLoops, unsigned delay, unsigned duplicateVns = 0, bool catchUp = false) {
    std::cout << "updaterLoop: delay=" << delay << ", duplicateVns=" << duplicateVns << std::endl;
    auto accA = dev.getScalarRegisterAccessor<int32_t>("/A");
    auto accB = dev.getScalarRegisterAccessor<int32_t>("/B");
    // note, the default-constructed VersionNumbers in here will not be used
    std::vector<ChimeraTK::VersionNumber> vs(nLoops);
    for(unsigned loopCount = 0; loopCount < nLoops; loopCount++) {
      std::cout << "updaterLoop: writing value " << loopCount << std::endl;
      accA = (int32_t)loopCount;
    retryWrite:
      // device might be in error state so write can throw
      try {
        if(loopCount > 0 && duplicateVns > 0) {
          vs[loopCount] = vs[loopCount - 1]; // use last VersionNumber another time
          duplicateVns--;
        }
        else {
          vs[loopCount] = {}; // new VersionNumber
        }
        accA.write(vs[loopCount]);
        if(loopCount >= delay) {
          accB = (int32_t)(loopCount - delay);
          accB.write(vs[loopCount - delay]);
        }
        if(loopCount == nLoops - 1 && delay > 0 && catchUp) {
          // let variable B catch up with A
          for(unsigned i = loopCount - delay + 1; i < nLoops; i++) {
            accB = (int32_t)(i);
            accB.write(vs[i]);
          }
        }
      }
      catch(ChimeraTK::runtime_error& e) {
        std::cout << "updaterLoop: exception, wait and retry write.." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds{500});
        goto retryWrite;
      }
      // wait on data receive before writing next
      waitOnReceiveAck();
    }
    // interrupt blocking reads in test thread so it can terminate. It should be sufficient to
    // interrupt one of the read accessors.
    readAccA.interrupt();
  };

  Device dev;
  ScalarRegisterAccessor<int32_t> readAccA;
  ScalarRegisterAccessor<int32_t> readAccB;

  // we use a semaphore to acknowlegde data received; like that we avoid future_queue overruns
  sem_t sem;
};

BOOST_FIXTURE_TEST_CASE(test1, Fixture) {
  // minimal test code: we create two read accessors on variables defined in xlmap, put them into a ReadAnyGroup and a
  // historized DataConsistencyGroup. We provide data from another thread, as in real use case. Then using different
  // delay settings for updates of A and B, check expected number of consistent data updates.

  ChimeraTK::ReadAnyGroup rag{readAccA, readAccB};

  std::cout << "test1: no history" << std::endl;
  // without historized DataConsistencyGroup, check that we get consistent updates only if delay is 0
  const unsigned nLoops = 4;
  for(unsigned delay = 0; delay <= 2; delay++) {
    emptyQueues(rag);
    unsigned nUpdates = 0;
    unsigned nConsistentUpdates = 0;
    std::thread updaterThread1([&]() { updaterLoop(nLoops, delay); });
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

  std::cout << "test1: with history" << std::endl;
  DataConsistencyGroup dg({readAccA, readAccB}, DataConsistencyGroup::MatchingMode::historized);

  for(unsigned delay = 0; delay <= 2; delay++) {
    // With HDataConsistencyGroup, check that we get N-delay consistent updates.
    // Also check that we have consistent data (e.g. data=versionnumber counter)
    emptyQueues(rag, &dg);
    unsigned nUpdates = 0;
    unsigned nConsistentUpdates = 0;
    std::thread updaterThread1([&]() { updaterLoop(nLoops, delay); });
    // test loop consuming data
    try {
      while(true) {
        auto id = rag.readAny();
        bool isConsistent = dg.update(id);
        nUpdates++;
        if(readAccA.getVersionNumber() == readAccB.getVersionNumber()) {
          nConsistentUpdates++;
        }
        // check data consistency via VersionNumber and content
        BOOST_TEST(isConsistent);
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

BOOST_FIXTURE_TEST_CASE(testDuplicateVns, Fixture) {
  std::cout << "testDuplicateVns" << std::endl;

  ChimeraTK::ReadAnyGroup rag{readAccA, readAccB};

  DataConsistencyGroup dg({readAccA, readAccB}, DataConsistencyGroup::MatchingMode::historized);

  const unsigned nLoops = 4;
  const unsigned nDuplicateVns = 1;

  for(unsigned delay = 0; delay <= 2; delay++) {
    // With MatchingMode::historized, check that we get N-delay consistent updates.
    // Also check that we have consistent data (e.g. data=versionnumber counter)
    emptyQueues(rag, &dg);
    unsigned nUpdates = 0;
    unsigned nConsistentUpdates = 0;
    std::thread updaterThread1([&]() { updaterLoop(nLoops, delay, nDuplicateVns); });
    // test loop consuming data
    try {
      while(true) {
        auto id = rag.readAny();
        nUpdates++;
        bool isConsistent = dg.update(id);
        if(readAccA.getVersionNumber() == readAccB.getVersionNumber()) {
          nConsistentUpdates++;
        }
        // check data consistency via VersionNumber
        BOOST_TEST(isConsistent);
        BOOST_TEST(readAccA.getVersionNumber() == readAccB.getVersionNumber());
        // acknowledge data received: here updated id is irrelevant
        sem_post(&sem);
      }
    }
    catch(boost::thread_interrupted& e) {
      std::cout << "thread interrupted" << std::endl;
    }
    updaterThread1.join();
    unsigned nExpectedUpdates = nLoops - delay;
    if(delay == 0) {
      // for each VersionNumber that is repeated for A, we should get one extra consistent update
      nExpectedUpdates++;
    }
    BOOST_TEST(nConsistentUpdates == nExpectedUpdates);
    BOOST_TEST(nConsistentUpdates == nUpdates);
  }
}

BOOST_FIXTURE_TEST_CASE(testExceptions, Fixture) {
  std::cout << "testExceptions" << std::endl;

  ChimeraTK::ReadAnyGroup rag{readAccA, readAccB};
  DataConsistencyGroup dg({readAccA, readAccB}, DataConsistencyGroup::MatchingMode::historized);

  const unsigned nLoops = 6;
  const unsigned delay = 0;
  {
    // With HDataConsistencyGroup, check that we get N-delay consistent updates.
    // Also check that we have consistent data (e.g. data=versionnumber counter)
    emptyQueues(rag, &dg);
    unsigned nUpdates = 0;
    unsigned nConsistentUpdates = 0;
    std::thread updaterThread1([&]() { updaterLoop(nLoops, delay); });
  retry:
    // test loop consuming data
    try {
      while(true) {
        auto id = rag.readAny();
        auto& acc = id == readAccA.getId() ? readAccA : readAccB;
        std::cout << "readAny: seeing update for target " << acc.getName() << " vs " << acc.getVersionNumber()
                  << " values " << readAccA << "," << readAccB << std::endl;

        bool isConsistent = dg.update(id);
        nUpdates++;
        if(readAccA.getVersionNumber() == readAccB.getVersionNumber()) {
          nConsistentUpdates++;
        }
        // check data consistency via VersionNumber and content
        BOOST_TEST(isConsistent);
        BOOST_TEST(readAccA.getVersionNumber() == readAccB.getVersionNumber());
        BOOST_TEST(readAccA == readAccB);

        if(nUpdates == 2) {
          // shortly put device into exception state and recover. When the exception is seen by the accessors,
          // it should pop out from readAny but after that, we should be able to continue normal operation (at retry:)
          dev.setException("exception");
          dev.open();
          dev.activateAsyncRead();
        }

        // acknowledge data received: here updated id is irrelevant
        sem_post(&sem);
      }
    }
    catch(boost::thread_interrupted& e) {
      std::cout << "thread interrupted" << std::endl;
    }
    catch(ChimeraTK::runtime_error& e) {
      std::cout << "runtime error: " << e.what() << std::endl;
      goto retry;
    }

    updaterThread1.join();
    // one more update since after exception, open, activateAsyncRead, we get another initial value
    BOOST_TEST(nConsistentUpdates == nLoops - delay + 1);
    BOOST_TEST(nConsistentUpdates == nUpdates);
  }
}

BOOST_FIXTURE_TEST_CASE(testCatchUp, Fixture) {
  std::cout << "testCatchUp" << std::endl;

  ChimeraTK::ReadAnyGroup rag{readAccA, readAccB};
  DataConsistencyGroup dg({readAccA, readAccB}, DataConsistencyGroup::MatchingMode::historized);

  const unsigned nLoops = 6;
  const unsigned delay = 2;

  // With HDataConsistencyGroup, check that we get N consistent updates, even when we
  // have update delay on second variable that appears and vanishes again.
  emptyQueues(rag, &dg);
  unsigned nUpdates = 0;
  unsigned nConsistentUpdates = 0;
  std::thread updaterThread1([&]() { updaterLoop(nLoops, delay, false, true); });

  // test loop consuming data
  try {
    while(true) {
      auto id = rag.readAny();
      auto& acc = id == readAccA.getId() ? readAccA : readAccB;
      std::cout << "readAny: seeing update for target " << acc.getName() << " vs " << acc.getVersionNumber()
                << " values " << readAccA << "," << readAccB << std::endl;

      bool isConsistent = dg.update(id);
      nUpdates++;
      if(readAccA.getVersionNumber() == readAccB.getVersionNumber()) {
        nConsistentUpdates++;
      }
      // check data consistency via VersionNumber and content
      BOOST_TEST(isConsistent);
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
  BOOST_TEST(nConsistentUpdates == nLoops);
  BOOST_TEST(nConsistentUpdates == nUpdates);
}

BOOST_FIXTURE_TEST_CASE(testInitialValues, Fixture) {
  std::cout << "testInitialValues" << std::endl;
  // at start VersionNum(A)=0, since no read has yet occurred

  ChimeraTK::ReadAnyGroup rag{readAccA, readAccB};
  DataConsistencyGroup dg({readAccA, readAccB}, DataConsistencyGroup::MatchingMode::historized);

  unsigned nDiscarded = emptyQueues(rag);
  // after read, VersionNumbers must be non-zero.
  // Note, initial values here count as one consistent set.
  BOOST_TEST(nDiscarded == 1);

  BOOST_TEST(readAccA.getVersionNumber() != VersionNumber(nullptr));
  BOOST_TEST(readAccB.getVersionNumber() != VersionNumber(nullptr));
}

BOOST_AUTO_TEST_CASE(testInitialValuesConsistency) {
  // in all the previous tests, we simply discarded the initial values.
  // However in real use, e.g. with ApplicationCore, it often makes sense to keep the initial values and complete
  // them with some data update that turns them into a consistent set.
  // Test that MatchingMode::historized supports this use case.

  Device dev;
  dev.open("(logicalNameMap?map=historizedDataMatching.xlmap)");
  dev.activateAsyncRead();

  for(bool extraDecorators : {false, true}) {
    // prepare initial values
    VersionNumber vs0;
    VersionNumber vs1;
    auto accA = dev.getScalarRegisterAccessor<int32_t>("/A");
    auto accB = dev.getScalarRegisterAccessor<int32_t>("/B");
    accA.setAndWrite(100, vs1);
    accB.setAndWrite(99, vs0);

    // use 'fresh' read accessors not yet tainted by ReadAnyGroup or DataConsistencyGroup
    auto readAccA = dev.getScalarRegisterAccessor<int32_t>("/A", 0, {AccessMode::wait_for_new_data});
    auto readAccB = dev.getScalarRegisterAccessor<int32_t>("/B", 0, {AccessMode::wait_for_new_data});

    if(extraDecorators) {
      // In order to mimic ApplicationCore behaviour, where a MetaDataPropagatingRegisterDecorator is placed around
      // every accessor, we add a decoration layer via otherwise useless NDRegisterAccessorDecorator.
      // We want to test that DataConsistencyDecorator swaps the right buffers even then.
      auto da = boost::make_shared<NDRegisterAccessorDecorator<int32_t>>(readAccA.getImpl());
      readAccA.replace(da);
      auto db = boost::make_shared<NDRegisterAccessorDecorator<int32_t>>(readAccB.getImpl());
      readAccB.replace(db);
    }

    // read and check them
    readAccA.readLatest();
    readAccB.readLatest();
    BOOST_TEST(readAccA == 100);
    BOOST_TEST(readAccA.getVersionNumber() == vs1);
    BOOST_TEST(readAccB == 99);
    BOOST_TEST(readAccB.getVersionNumber() == vs0);

    ReadAnyGroup rag{readAccA, readAccB};
    DataConsistencyGroup dg({readAccA, readAccB}, DataConsistencyGroup::MatchingMode::historized);
    // check user buffer again - because of DataConsistencyDecorator
    BOOST_TEST(readAccA.getVersionNumber() == vs1);
    BOOST_TEST(readAccB.getVersionNumber() == vs0);
    BOOST_TEST(dg.isConsistent() == false);
    // note, following 2 tests fail unless DataConsistencyDecorator takes over initial data on construction
    BOOST_TEST(readAccA == 100);
    BOOST_TEST(readAccB == 99);

    // provide data update for B that completes consistent set
    accB.setAndWrite(100, vs1);
    auto id = rag.readAny();
    BOOST_TEST(id == readAccB.getId());
    BOOST_TEST(readAccA == 100);
    BOOST_TEST(readAccB == 100);
    BOOST_TEST(readAccA.getVersionNumber() == vs1);
    BOOST_TEST(readAccB.getVersionNumber() == vs1);
    BOOST_TEST(dg.isConsistent() == true);
  }
}

BOOST_FIXTURE_TEST_CASE(testIllegalUse, Fixture) {
  std::cout << "testIllegalUse" << std::endl;

  ChimeraTK::ReadAnyGroup rag{readAccA, readAccB};
  DataConsistencyGroup dg({readAccA, readAccB}, DataConsistencyGroup::MatchingMode::historized);

  BOOST_CHECK_THROW(
      DataConsistencyGroup({readAccA}, DataConsistencyGroup::MatchingMode::historized), ChimeraTK::logic_error);
}

BOOST_AUTO_TEST_SUITE_END()
