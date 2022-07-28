// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#warning Using DMapFile is deprecated. The class has been renamed to DeviceInfoMap.

#include "DeviceInfoMap.h"

namespace ChimeraTK {

  /** Using DMapFile is deprecated. The class has been renamed to DeviceInfoMap.
   *  @deprecated Using DMapFile is deprecated. The class has been renamed to
   * DeviceInfoMap.
   */
  class DMapFile : public DeviceInfoMap {
   public:
    /** The name DRegisterInfo was a misnomer due to refactoring. Use DeviceInfo
     * instead. This alias does not make DRegisterInfo work the way it did because
     * the members have also been renamed.
     * @deprecated The name DRegisterInfo was a misnomer due to refactoring. Use
     * DeviceInfo instead.
     */
    typedef DeviceInfo DRegisterInfo;
  };

} // namespace ChimeraTK
