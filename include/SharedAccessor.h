// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NDRegisterAccessor.h"

#include <mutex>

namespace ChimeraTK::detail {
  /** Struct holding shared accessors together with a mutex for thread safety. See sharedAccessorMap data member. */
  template<typename UserType>
  struct SharedAccessor {
    boost::weak_ptr<NDRegisterAccessor<UserType>> accessor;
    std::recursive_mutex mutex;
  };

  /** Map of target accessors which are potentially shared across accessors. An example is the target accessors of
   *  LNMBackendBitAccessor. Multiple instances of LNMBackendBitAccessor referring to different bits of the same
   *  register share their target accessor. This sharing is governed by this map. */
  using SharedAccessorKey = std::pair<DeviceBackend*, RegisterPath>;

  template<typename UserType>
  using SharedAccessorMap = std::map<SharedAccessorKey, SharedAccessor<UserType>>;

} // namespace ChimeraTK::detail
