// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

// Utilities to manage Linux processes
#include <string>

bool processExists(unsigned pid);
unsigned getOwnPID();
std::string getUserName();
