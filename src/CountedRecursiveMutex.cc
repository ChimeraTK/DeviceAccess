// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CountedRecursiveMutex.h"

#include <cassert>

namespace ChimeraTK::detail {

  /********************************************************************************************************************/

  size_t CountedRecursiveMutex::useCount() const {
    return _useCount;
  }

  /********************************************************************************************************************/

  void CountedRecursiveMutex::unlock() {
    assert(_useCount > 0);
    _useCount--;
    std::recursive_mutex::unlock();
  }
  /********************************************************************************************************************/

  void CountedRecursiveMutex::lock() {
    std::recursive_mutex::lock();
    _useCount++;
  }

  /********************************************************************************************************************/

  bool CountedRecursiveMutex::try_lock() {
    if(std::recursive_mutex::try_lock()) {
      _useCount++;
      return true;
    }
    return false;
  }

  /********************************************************************************************************************/
} // namespace ChimeraTK::detail
