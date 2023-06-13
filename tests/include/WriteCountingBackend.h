// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DummyBackend.h"

using namespace ChimeraTK;

/**********************************************************************************************************************/

struct WriteCountingBackend : public DummyBackend {
  using DummyBackend::DummyBackend;

  static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string> parameters) {
    return returnInstance<WriteCountingBackend>(
        parameters.at("map"), convertPathRelativeToDmapToAbs(parameters.at("map")));
  }

  size_t writeCount{0};

  void write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) override {
    ++writeCount;
    DummyBackend::write(bar, address, data, sizeInBytes);
  }

  struct BackendRegisterer {
    BackendRegisterer() {
      ChimeraTK::BackendFactory::getInstance().registerBackendType(
          "WriteCountingDummy", &WriteCountingBackend::createInstance, {"map"});
    }
  };
};

static WriteCountingBackend::BackendRegisterer gWriteCountingBackendRegisterer;

/**********************************************************************************************************************/
