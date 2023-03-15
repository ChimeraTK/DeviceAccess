// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "RegisterPath.h"

namespace ChimeraTK::numeric_address {

  /** The numeric_address::BAR() function can be used to directly access registers
   * by numeric addresses, instead of using register names (e.g. when no map file
   * exists). The address will form a special RegisterPath which can be used in
   * any place where a register path name is expected, if the backend supports it.
   *
   * The syntax is as follows
   *
   * - `BAR()/<barNumber>/<addressInBytes>` -> access 4-byte
   * register in bar `<barNumber>` at byte offset `<addressInBytes>`
   * - `BAR()/<barNumber>/<addressInBytes>*<lengthInBytes>` -> access register with
   * arbitrary length of `<lengthInBytes>`
   */
  RegisterPath BAR();
} // namespace ChimeraTK::numeric_address
