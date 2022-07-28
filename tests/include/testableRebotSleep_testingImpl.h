// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <boost/chrono.hpp>
#include <boost/thread.hpp>

#include <atomic>
#include <iostream>
#include <mutex>

typedef boost::chrono::steady_clock::time_point TimePoint;

namespace ChimeraTK {
  class RebotTestableClock {
   public:
    static boost::chrono::steady_clock::time_point now() {
      std::cout << "TestIMPL:: returning now = " << (_epoch - _now).count() << std::endl;
      return _now;
    }
    static boost::chrono::steady_clock::time_point _now;
    static boost::chrono::steady_clock::time_point _epoch;

    template<class Rep, class Period>
    static void setTime(boost::chrono::duration<Rep, Period> timeSinceMyEpoch) {
      _now = _epoch + timeSinceMyEpoch;
    }
  };
  TimePoint RebotTestableClock::_epoch(boost::chrono::steady_clock::now());
  TimePoint RebotTestableClock::_now(RebotTestableClock::_epoch);

  // In a future implementation we might want to hold several synchronisers (one
  // for each thread) in a loopup table. For now we the members static.
  struct RebotSleepSynchroniser {
    static std::mutex _lock;
    // only modify this variable while holding the lock. You may read it without, that's why it's atomic.
    static std::atomic<bool> _clientMayGetLock;
    // may only be accessed while holding the lock.
    static boost::chrono::steady_clock::time_point _nextRequestedWakeup;
    // may only be modified once (set to true) by the client code (in sleep_until)
    static std::atomic<bool> _clientHasReachedTestableMode;

    // only used in the test thread
    static bool _testHasReachedTestableMode;
  };

  std::mutex RebotSleepSynchroniser::_lock;
  std::atomic<bool> RebotSleepSynchroniser::_clientMayGetLock(false);
  boost::chrono::steady_clock::time_point RebotSleepSynchroniser::_nextRequestedWakeup = RebotTestableClock::_epoch;
  std::atomic<bool> RebotSleepSynchroniser::_clientHasReachedTestableMode(false);
  bool RebotSleepSynchroniser::_testHasReachedTestableMode(false);

  namespace testable_rebot_sleep {
    boost::chrono::steady_clock::time_point now() {
      std::cout << "function TestIMPL:: returning now = "
                << (RebotTestableClock::_now - RebotTestableClock::_epoch).count() << std::endl;
      return RebotTestableClock::_now;
    }

    /** There are two implementations with the same signature:
     *  One that calls boost::thread::this_thread::sleep_until, which is used in the
     * application. The one for testing has a lock and is synchronised manually with
     * the test thread.
     */
    void sleep_until(boost::chrono::steady_clock::time_point t) {
      RebotSleepSynchroniser::_nextRequestedWakeup = t;

      // The application is done with whatever it was doing and going to sleep.
      // This is the synchronisation point where we hand the lock back to the
      // test thread.
      if(RebotSleepSynchroniser::_clientHasReachedTestableMode) {
        RebotSleepSynchroniser::_lock.unlock();
      }
      else {
        // We don't hold the lock yet, so we cannot unlock it. But the next time we reach this place we will.
        RebotSleepSynchroniser::_clientHasReachedTestableMode = true;
      }

      std::cout << "application unlocked" << std::endl;

      // Yield the thread (give away the rest of the time slice) until we are
      // allowed to hold the lock. The actual waiting for execution is happening in
      // the lock. This flag is only used to avoid the race condition that the
      // application tries to lock before the test thread had the change to aquire
      // the lock. For a proper handshake both test code and application must have
      // locked before the other side relocks.
      std::cout << "application yielding..." << std::endl;
      do {
        boost::this_thread::interruption_point();
        boost::this_thread::yield();
      } while(!RebotSleepSynchroniser::_clientMayGetLock);

      std::cout << "application trying to lock" << std::endl;
      RebotSleepSynchroniser::_lock.lock();
      // now that we are holding the lock we set _clientMayGetLock to false and
      // wait for the test thread to signal us that we can take the lock again
      RebotSleepSynchroniser::_clientMayGetLock = false;
      std::cout << "application locked, mayget is false" << std::endl;
    }

    // don't use this directly, only through advance_until
    void wake_up_application() {
      RebotSleepSynchroniser::_lock.unlock();
      std::cout << "test unlocked" << std::endl;

      // the client must signal that it acquired the lock, otherwise we do not know
      // if it excecuted its task or not. As long as the client is still allowed to
      // get the lock, it has not had it, and we don't get it again.
      std::cout << "test yielding..." << std::endl;
      do {
        boost::this_thread::yield();
      } while(RebotSleepSynchroniser::_clientMayGetLock);

      std::cout << "test trying to lock" << std::endl;
      RebotSleepSynchroniser::_lock.lock();
      // Signal to the client that you were holding the lock and the synchronisation
      // is done. So the next time it checks the lock it is allowed to hold it.
      RebotSleepSynchroniser::_clientMayGetLock = true;
      // The client will block, trying to get the lock and woken up by the scheduler
      // when it is freed again.
      std::cout << "test locked, mayget is true" << std::endl;
    }

    template<class Rep, class Period>
    void advance_until(boost::chrono::duration<Rep, Period> targetTimeRelativeMyEpoch) {
      assert(RebotSleepSynchroniser::_testHasReachedTestableMode);
      auto absoluteTargetTime = RebotTestableClock::_epoch + targetTimeRelativeMyEpoch;
      std::cout << "advanving to " << (absoluteTargetTime - RebotTestableClock::_epoch).count() << std::endl;
      std::cout << "next wakeup requested for "
                << (RebotSleepSynchroniser::_nextRequestedWakeup - RebotTestableClock::_epoch).count() << std::endl;

      while(RebotTestableClock::_now < absoluteTargetTime) {
        if(RebotSleepSynchroniser::_nextRequestedWakeup <= absoluteTargetTime) {
          RebotTestableClock::_now = RebotSleepSynchroniser::_nextRequestedWakeup;
          wake_up_application();
        }
        else {
          RebotTestableClock::_now = absoluteTargetTime;
        }
      }
    }

    // The client always starts without the lock because it starts a new thread and is only interacting with
    // sleep_until(). So we have to wait in the test until the is waiting in sleep_until, then get the lock.
    void waitForClientTestableMode() {
      boost::this_thread::yield();
      while(!RebotSleepSynchroniser::_clientHasReachedTestableMode) {
        boost::this_thread::yield();
      }
      RebotSleepSynchroniser::_lock.lock();
      RebotSleepSynchroniser::_clientMayGetLock = true; // client may get the lock the next time the tests releases it.
      RebotSleepSynchroniser::_testHasReachedTestableMode = true; // make sure this function is called first by an
                                                                  // assertion in advance_until.
      std::cout << "test locked initially , mayget is true" << std::endl;
    }

  } // namespace testable_rebot_sleep
} // namespace ChimeraTK
