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
   * - `BAR()/<barNumber>/<addressInBytes>*<nBytes>u<bitWidth>` -> access an unsigned
   * register of `<nBytes>` bytes whose element width is `<bitWidth>` (8, 16, 32 or 64)
   * - `BAR()/<barNumber>/<addressInBytes>*<nBytes>u<bitWidth>p<pitchBits>` -> access a
   * strided register of `<nBytes>` bytes: the element pitch is `<pitchBits>` bits (a
   * multiple of 8) and the number of elements is `<nBytes>*8/<pitchBits>`
   *
   * Every part after `<addressInBytes>` is optional: `*<nBytes>` is the total span in
   * bytes (default: the element size), `u<bitWidth>` selects an unsigned and
   * `s<bitWidth>` a signed element of width 8, 16, 32 or 64 (default: signed 32-bit),
   * and `p<pitchBits>` sets the element pitch in bits (a multiple of 8). With
   * `p<pitchBits>` the register is strided: each element has a distance of
   * `pitchBits` bits in memory, and `nElements = nBytes*8/pitchBits`.
   */
  RegisterPath BAR();
} // namespace ChimeraTK::numeric_address
