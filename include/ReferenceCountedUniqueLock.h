// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <mutex>

namespace ChimeraTK::detail {
  /**
   Helper class that keeps track of how many locks were taken on the recursive mutex in the current thread.

   It is used by the BitRange accessor to determine whether or not it can safely call read() on the target in
   preWrite for read-modify-write operations
  */
  class ReferenceCountedUniqueLock {
   public:
    ReferenceCountedUniqueLock() = default;

    explicit ReferenceCountedUniqueLock(std::recursive_mutex& mutex) : _lock(mutex, std::defer_lock) {}

    void lock();
    void unlock();
    [[nodiscard]] size_t useCount() const;

   private:
    thread_local static size_t targetUseCount;
    std::unique_lock<std::recursive_mutex> _lock;
  };

} // namespace ChimeraTK::detail
