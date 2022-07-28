// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <cstdint>
#include <vector>

/// Only put commands which don't exist in all versions, or behave differently
struct DummyProtocolImplementor {
  virtual void singleWordWrite(std::vector<uint32_t>& buffer) = 0;
  virtual void multiWordRead(std::vector<uint32_t>& buffer) = 0;
  // returns a new state: it might be that the next packet is still from the
  // same write operation, so we are not ready for a new command.
  virtual uint32_t multiWordWrite(std::vector<uint32_t>& buffer) = 0;
  virtual uint32_t continueMultiWordWrite(std::vector<uint32_t>& buffer) = 0;

  virtual void hello(std::vector<uint32_t>& buffer) = 0;
  virtual void ping(std::vector<uint32_t>& buffer) = 0;
  /// implement this for EVERY protocol version
  virtual uint32_t protocolVersion() const = 0;

  virtual ~DummyProtocolImplementor() = default;
};
