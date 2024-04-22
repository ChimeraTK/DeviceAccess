// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ExperimentalFeatures.h"

namespace ChimeraTK {
  std::atomic<bool> ExperimentalFeatures::isEnabled{false};
  ExperimentalFeatures::Reminder ExperimentalFeatures::reminder;
} // namespace ChimeraTK
