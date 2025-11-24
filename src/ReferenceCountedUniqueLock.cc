// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ReferenceCountedUniqueLock.h"

#include <cassert>

namespace ChimeraTK::detail {

  /********************************************************************************************************************/

  thread_local size_t ReferenceCountedUniqueLock::targetUseCount;

  /********************************************************************************************************************/

  size_t ReferenceCountedUniqueLock::useCount() const {
    assert(_lock.owns_lock());
    return targetUseCount;
  }

  /********************************************************************************************************************/

  void ReferenceCountedUniqueLock::unlock() {
    assert(targetUseCount > 0);
    targetUseCount--;
    _lock.unlock();
  }
  /********************************************************************************************************************/

  void ReferenceCountedUniqueLock::lock() {
    _lock.lock();
    targetUseCount++;
  }

  /********************************************************************************************************************/
} // namespace ChimeraTK::detail
