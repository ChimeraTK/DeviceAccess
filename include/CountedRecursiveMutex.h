// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <atomic>
#include <mutex>

namespace ChimeraTK::detail {

  class CountedRecursiveMutex : public std::recursive_mutex {
   public:
    CountedRecursiveMutex() = default;

    void lock();
    void unlock();
    bool try_lock();

    // This count is only reliable when holding the lock.
    [[nodiscard]] size_t useCount() const;

   private:
    std::atomic<size_t> _useCount{0};
  };

} // namespace ChimeraTK::detail
