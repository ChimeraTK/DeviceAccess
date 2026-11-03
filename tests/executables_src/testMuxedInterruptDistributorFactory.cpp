// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE MuxedInterruptDistributorFactoryTest
#include "async/MuxedInterruptDistributor.h"
#include "Device.h"
#include "MapFileParser.h"

#include <boost/test/unit_test.hpp>

#include <unistd.h>

#include <fstream>
#include <set>
#include <string>

using namespace ChimeraTK;
using namespace boost::unit_test_framework;

namespace {

  /********************************************************************************************************************/

  /** Stable description of a catalogue: the sorted register paths and the set of interrupt ids. */
  struct CatalogueSnapshot {
    std::set<std::string> registerPaths;
    std::set<std::vector<size_t>> interrupts;
  };

  /********************************************************************************************************************/

} // namespace

static CatalogueSnapshot snapshot(const NumericAddressedRegisterCatalogue& catalogue) {
  CatalogueSnapshot result;
  for(const auto& info : catalogue) {
    result.registerPaths.insert(info.getRegisterName());
  }
  result.interrupts = catalogue.getListOfInterrupts();
  return result;
}

static void checkUnchanged(const CatalogueSnapshot& before, const CatalogueSnapshot& after) {
  BOOST_TEST(after.registerPaths == before.registerPaths);
  BOOST_TEST(after.interrupts == before.interrupts);
}

BOOST_AUTO_TEST_SUITE(MuxedInterruptDistributorFactoryTestSuite)

/**********************************************************************************************************************/

// The dispatcher is a no-op for a controller type it does not know and for the built-in "dummy" type, which has no
// augmenter. Both must leave the catalogue untouched.
BOOST_AUTO_TEST_CASE(testAugmentCatalogueNoOp) {
  auto [catalogue, metas] = MapFileParser::parse("simpleJsonFile.jmap");
  (void)metas;

  auto before = snapshot(catalogue);
  BOOST_TEST(!before.registerPaths.empty()); // guard: the fixture really carries registers
  BOOST_TEST(!before.interrupts.empty());

  auto& factory = async::MuxedInterruptDistributorFactory::getInstance();

  // Unknown controller type: there is no record, so the entry is skipped.
  factory.augmentCatalogue({0}, "NoSuchControllerType", R"({"path": "DAQ"})", catalogue);
  checkUnchanged(before, snapshot(catalogue));

  // Built-in "dummy" type: there is a record but no augmenter, so the entry is skipped.
  factory.augmentCatalogue({0}, "dummy", R"({"module": "/some/module"})", catalogue);
  checkUnchanged(before, snapshot(catalogue));
}

/**********************************************************************************************************************/

// Creating a distributor for a controller type that is not registered throws, even though the dispatcher (which
// shares the record lookup) skips it silently. The type lookup happens before any ISR is opened, so a backend whose
// metadata declares an unknown type is enough.
BOOST_AUTO_TEST_CASE(testCreateUnknownControllerTypeThrows) {
  // A minimal map: metadata declares an unknown controller type for interrupt [0] and one register carries an
  // interrupt of the sub-handler [0, 0]. Requesting that sub-interrupt forces the creation of a distributor for
  // handler [0], which resolves the unregistered type.
  const std::string mapFile = "factoryUnknownController_" + std::to_string(getpid()) + ".mapp";
  {
    std::ofstream out(mapFile);
    out << R"(@![0] {"MyUnknownController":{"module":"/ctrl0"}})" << "\n"
        << R"(/ctrl0/active  1 0 4 0 32 0 0 RW)" << "\n"
        << R"(/myinterrupt   0 0 0 0 0 0 0 INTERRUPT0:0)" << "\n";
  }

  Device device{"(dummy?map=" + mapFile + ")"};
  device.open();
  device.activateAsyncRead();

  BOOST_CHECK_THROW(
      (void)device.getVoidRegisterAccessor("!0:0", {ChimeraTK::AccessMode::wait_for_new_data}), ChimeraTK::logic_error);

  device.close();
  std::remove(mapFile.c_str());
}

/**********************************************************************************************************************/

// The descriptor validation of getInterruptControllerNameAndDescription also guards the metadata catalogue path, which
// a traditional map file uses: the metadata carries the controller descriptor verbatim and never passes through the
// JSON entry reader. A descriptor with several controller keys is a malformed entry and must make the distributor
// creation throw rather than silently picking the first key.
//
// The map is a complete INTC register set for handler [0] (its ISR, the interface-enable register and a 1-bit child of
// ISR), so with the check dropped the entry would resolve the first controller key to a fully usable "INTC"
// distributor and the interrupt accessor would be created successfully. The map deliberately does not lean on the
// register lookup to fail for an unrelated reason, so the test fails exactly when the descriptor check is absent.
BOOST_AUTO_TEST_CASE(testCreateSeveralControllerKeysThrows) {
  const std::string mapFile = "factorySeveralControllerKeys_" + std::to_string(getpid()) + ".mapp";
  {
    std::ofstream out(mapFile);
    out << R"(@![0] {"INTC":{"path":"TEST0","version":1},"OTHER":{"path":"TEST0"}})" << "\n"
        << R"(TEST0.IER        1 0x8000C 4 0 7 0 0 RW)" << "\n"
        << R"(TEST0.ISR        1 0x80008 4 0 7 0 0 RW)" << "\n"
        << R"(TEST0.DAQ_READY  0 0       0 0 0 0 0 INTERRUPT0:4)" << "\n";
  }

  BOOST_CHECK_THROW(
      [&] {
        Device device{"(dummy?map=" + mapFile + ")"};
        device.open();
        device.activateAsyncRead();
        (void)device.getVoidRegisterAccessor("!0:4", {ChimeraTK::AccessMode::wait_for_new_data});
      }(),
      ChimeraTK::logic_error);

  std::remove(mapFile.c_str());
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
