// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "BackendFactory.h"
#include "DeviceAccessVersion.h"
#include "DummyBackend.h"

using namespace ChimeraTK;

struct AnotherWorkingBackend : public DummyBackend {
  using DummyBackend::DummyBackend;

  static boost::shared_ptr<DeviceBackend> createInstance(
      std::string instance, std::map<std::string, std::string> parameters) {
    return returnInstance<AnotherWorkingBackend>(instance, convertPathRelativeToDmapToAbs(parameters["map"]));
  }

  struct BackendRegisterer {
    BackendRegisterer() {
      ChimeraTK::BackendFactory::getInstance().registerBackendType(
          "another", &AnotherWorkingBackend::createInstance, {"map"}, CHIMERATK_DEVICEACCESS_VERSION);
    }
  };
};

static AnotherWorkingBackend::BackendRegisterer gAnotherWorkingBackendRegisterer;

extern "C" {
const char* deviceAccessVersionUsedToCompile() {
  return CHIMERATK_DEVICEACCESS_VERSION;
}
}
