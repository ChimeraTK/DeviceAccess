// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <boost/chrono.hpp>
#include <boost/thread.hpp>

namespace ChimeraTK { namespace testable_rebot_sleep {
  boost::chrono::steady_clock::time_point now();

  /** There are two implementations with the same signature:
   *  One that calls boost::thread::this_thread::sleep_until, which is used in the
   * application. The one for testing has a lock and is synchronised manually with
   * the test thread.
   */
  void sleep_until(boost::chrono::steady_clock::time_point t);
}} // namespace ChimeraTK::testable_rebot_sleep
