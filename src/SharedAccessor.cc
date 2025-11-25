// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SharedAccessor.h"

namespace ChimeraTK::detail {

  std::unique_ptr<SharedAccessorMap> SharedAccessorMap::_impl;

  SharedAccessorMap& SharedAccessorMap::getInstance() {
    if(!_impl) {
      _impl = std::make_unique<SharedAccessorMap>();
    }
    return *_impl;
  }

} // namespace ChimeraTK::detail
