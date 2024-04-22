// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "testableRebotSleep.h"

namespace ChimeraTK::testable_rebot_sleep {
  boost::chrono::steady_clock::time_point now() {
    return boost::chrono::steady_clock::now();
  }

  /** There are two implementations with the same signature:
   *  One that calls boost::thread::this_thread::sleep_until, which is used in the
   * application. The one for testing has a lock and is synchronised manually with
   * the test thread.
   */
  void sleep_until(boost::chrono::steady_clock::time_point t) {
    boost::this_thread::sleep_until(t);
  }
} // namespace ChimeraTK::testable_rebot_sleep
