// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>

namespace ChimeraTK {

  /** This is the base class of the code which implements the ReboT protocol.
   *  Starting from version 0, the implementors can derrive from each other to
   *  reuse code from previous versions, or replace the implementation.
   */
  struct RebotProtocolImplementor {
    virtual void read(uint32_t addressInBytes, int32_t* data, size_t sizeInBytes) = 0;
    virtual void write(uint32_t addressInBytes, int32_t const* data, size_t sizeInBytes) = 0;
    virtual void sendHeartbeat() = 0;
    virtual ~RebotProtocolImplementor(){};
  };

} // namespace ChimeraTK
