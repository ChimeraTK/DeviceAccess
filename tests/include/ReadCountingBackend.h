// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DummyBackend.h"

using namespace ChimeraTK;

/**********************************************************************************************************************/

struct ReadCountingBackend : public DummyBackend {
  using DummyBackend::DummyBackend;

  static boost::shared_ptr<DeviceBackend> createInstance(
      std::string address, std::map<std::string, std::string> parameters) {
    return returnInstance<ReadCountingBackend>(address, parameters["map"]);
  }

  /// Total number of DeviceBackend-level reads performed since construction.
  size_t readCount{0};

  /// Number of reads of the physical data buffers only (bar 13, offsets 0x81000/0x81200). This counts only the actual
  /// data payload reads, decoupling the assertion from selector/controller register reads.
  size_t dataReadCount{0};

  void read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) override {
    ++readCount;
    if(bar == 13 && (address == 0x81000 || address == 0x81200)) {
      ++dataReadCount;
    }
    DummyBackend::read(bar, address, data, sizeInBytes);
  }

  struct BackendRegisterer {
    BackendRegisterer() {
      ChimeraTK::BackendFactory::getInstance().registerBackendType(
          "ReadCountingDummy", &ReadCountingBackend::createInstance, {"map"});
    }
  };
};

static ReadCountingBackend::BackendRegisterer gReadCountingBackendRegisterer;

/**********************************************************************************************************************/
