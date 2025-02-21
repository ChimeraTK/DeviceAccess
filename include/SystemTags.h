// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

namespace ChimeraTK {
  /**
   * A list of pre-defined tags that can be used to mark registers.
   * Other components (e.g. ApplicationCore) in the framework can make use of these tags to change
   * behavior towards those registers depending on presence of those tags.
   *
   * The structure of a system tag consists of three parts separated by "_":
   * _ChimeraTK_ModuleName_tagName.
   *
   * - The prefix "_ChimeraTK" classifies this as a system tag
   * - It is followed by a module name that can be used to group tags together
   * - The name of the tag
   */
  struct SystemTags {
    /// Used to mark something as aggregable by ApplicationCore's status aggregator.
    static constexpr auto statusOutput{"_ChimeraTK_StatusOutput_statusOutput"};
    /**
     * Used to flag a register as "do not recover". Use-case for this might be to flag registers
     * that immediately trigger actions when written to.
     */
    static constexpr auto skipOnDeviceRecovery{"_ChimeraTK_DeviceRegister_skipOnRecovery"};
  };
} // namespace ChimeraTK
