// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "BackendFactory.h"
#include "DummyBackend.h"

using namespace ChimeraTK;
#define WRONG_VERSION "00.18"

// LCOV_EXCL_START these lines cannot be reached because the backend cannot be
// registered, thus it is never instantiated
struct WrongVersionBackend : public DummyBackend {
  using DummyBackend::DummyBackend;
  static boost::shared_ptr<DeviceBackend> createInstance(
      std::string instance, std::map<std::string, std::string> parameters) {
    return returnInstance<WrongVersionBackend>(instance, convertPathRelativeToDmapToAbs(parameters.at("map")));
  }

  // LCOV_EXCL_STOP The registerern and the version functions have to be called

  struct BackendRegisterer {
    BackendRegisterer() {
      ChimeraTK::BackendFactory::getInstance().registerBackendType(
          "wrongVersionBackendCompat", &WrongVersionBackend::createInstance, {"map"}, WRONG_VERSION);
    }
  };
};

static WrongVersionBackend::BackendRegisterer gWrongVersionBackendRegisterer;

extern "C" {
const char* deviceAccessVersionUsedToCompile() {
  return WRONG_VERSION;
}
}
