// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK

#define BOOST_TEST_MODULE NumericAddressedRegisterCatalogue

#include "NumericAddressedRegisterCatalogue.h"

#include <boost/test/unit_test.hpp>

#include <optional>
#include <set>
#include <vector>

using namespace ChimeraTK;
using namespace boost::unit_test_framework;

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE(NumericAddressedRegisterCatalogueTestSuite)

/**********************************************************************************************************************/

// An interrupt register: a void register carrying a numeric interrupt id.
static NumericAddressedRegisterInfo makeInterruptRegister(
    const RegisterPath& path, const std::vector<size_t>& interruptId) {
  return NumericAddressedRegisterInfo(path, 0, 0, 0, 0, 0, 0, false, NumericAddressedRegisterInfo::Access::INTERRUPT,
      NumericAddressedRegisterInfo::Type::VOID, interruptId);
}

/**********************************************************************************************************************/

// The finalisation builds the interrupt list and the canonical interrupt paths of the interrupt registers present, and
// leaves a register without interrupt id out of both.
BOOST_AUTO_TEST_CASE(TestFinaliseBuildsInterruptIndexes) {
  NumericAddressedRegisterCatalogue catalogue;
  catalogue.addRegister(NumericAddressedRegisterInfo("REG", 1, 0x0, 4, 0, 32, 0, true));
  catalogue.addRegister(makeInterruptRegister("INT_3", {3}));
  catalogue.addRegister(makeInterruptRegister("INT_5_2", {5, 2}));

  catalogue.finaliseInterruptBookkeeping();

  const std::set<std::vector<size_t>> expectedInterrupts{{3}, {5, 2}};
  BOOST_TEST((catalogue.getListOfInterrupts() == expectedInterrupts));

  // Each canonical interrupt path of the hierarchical id exists and carries its id prefix.
  BOOST_TEST((catalogue.getBackendRegister("!3").numericInterruptId() == std::vector<size_t>{3}));
  BOOST_TEST((catalogue.getBackendRegister("!5").numericInterruptId() == std::vector<size_t>{5}));
  BOOST_TEST((catalogue.getBackendRegister("!5:2").numericInterruptId() == std::vector<size_t>{5, 2}));
  BOOST_TEST(catalogue.hasRegister("!5:2"));

  // A register without interrupt id is neither an interrupt nor a canonical interrupt path.
  BOOST_TEST((!catalogue.getBackendRegister("REG").numericInterruptId().has_value()));
  BOOST_TEST(!catalogue.hasRegister("!3:0"));
}

/**********************************************************************************************************************/

// A register with its own registered interrupt and a register deriving a second interrupt are both indexed, and the
// indexes are carried over to a clone of the catalogue.
BOOST_AUTO_TEST_CASE(TestFinaliseIndexesRegisteredAndDerivedInterruptsAndCloneCopy) {
  NumericAddressedRegisterCatalogue catalogue;
  catalogue.addRegister(makeInterruptRegister("REGISTERED", {1}));
  catalogue.addRegister(makeInterruptRegister("DERIVED", {2, 3}));

  catalogue.finaliseInterruptBookkeeping();

  const std::set<std::vector<size_t>> expectedInterrupts{{1}, {2, 3}};
  BOOST_TEST((catalogue.getListOfInterrupts() == expectedInterrupts));

  auto clone = catalogue.clone();
  auto* cloneCatalogue = dynamic_cast<NumericAddressedRegisterCatalogue*>(clone.get());
  BOOST_REQUIRE(cloneCatalogue != nullptr);

  // The clone is already finalised: both indexes are copied, so neither the interrupt list nor the canonical
  // interrupt paths need a further finalisation call.
  BOOST_TEST((cloneCatalogue->getListOfInterrupts() == expectedInterrupts));
  BOOST_TEST(cloneCatalogue->hasRegister("!2"));
  BOOST_TEST((cloneCatalogue->getBackendRegister("!2:3").numericInterruptId() == std::vector<size_t>{2, 3}));
}

/**********************************************************************************************************************/

// Construction without an interrupt id: a register built by the channel (2D) constructor from an empty id vector has no
// interrupt id, and neither has one built by the 1D/2D-agnostic constructor. Both constructors map the empty vector to
// std::nullopt, so a register the map file parsers give an empty id vector is not an interrupt source.
BOOST_AUTO_TEST_CASE(TestEmptyInterruptIdVectorConstructsRegisterWithoutInterrupt) {
  // The 2D (channel) constructor.
  NumericAddressedRegisterInfo channelRegister("CHANNEL_REGISTER", 0, 0, 4, 64,
      {{0, NumericAddressedRegisterInfo::Type::FIXED_POINT, 32, 0, true, ChimeraTK::DataType("int32")},
          {32, NumericAddressedRegisterInfo::Type::FIXED_POINT, 32, 0, true, ChimeraTK::DataType("int32")}},
      NumericAddressedRegisterInfo::Access::READ_WRITE, std::vector<size_t>{});
  BOOST_TEST(!channelRegister.interruptId.has_value());
  BOOST_TEST(!channelRegister.numericInterruptId().has_value());

  NumericAddressedRegisterInfo channelRegisterWithoutId("CHANNEL_REGISTER", 0, 0, 4, 64,
      {{0, NumericAddressedRegisterInfo::Type::FIXED_POINT, 32, 0, true, ChimeraTK::DataType("int32")},
          {32, NumericAddressedRegisterInfo::Type::FIXED_POINT, 32, 0, true, ChimeraTK::DataType("int32")}},
      NumericAddressedRegisterInfo::Access::READ_WRITE, std::nullopt, std::nullopt);
  BOOST_TEST((channelRegister == channelRegisterWithoutId));

  // The 1D/2D-agnostic constructor.
  NumericAddressedRegisterInfo scalarRegister("SCALAR_REGISTER", 1, 0x0, 4, 0, 32, 0, true,
      NumericAddressedRegisterInfo::Access::READ_WRITE, NumericAddressedRegisterInfo::Type::FIXED_POINT,
      std::vector<size_t>{});
  BOOST_TEST(!scalarRegister.interruptId.has_value());
  BOOST_TEST(!scalarRegister.numericInterruptId().has_value());

  NumericAddressedRegisterInfo scalarRegisterWithoutId("SCALAR_REGISTER", 1, 0x0, 4, 0, 32, 0, true,
      NumericAddressedRegisterInfo::Access::READ_WRITE, NumericAddressedRegisterInfo::Type::FIXED_POINT);
  BOOST_TEST((scalarRegister == scalarRegisterWithoutId));

  // A numeric id passes both constructors unchanged, so the mapping above only affects the empty vector.
  const std::vector<size_t> numericId{3};
  NumericAddressedRegisterInfo channelRegisterWithId("CHANNEL_REGISTER", 0, 0, 4, 64,
      {{0, NumericAddressedRegisterInfo::Type::FIXED_POINT, 32, 0, true, ChimeraTK::DataType("int32")},
          {32, NumericAddressedRegisterInfo::Type::FIXED_POINT, 32, 0, true, ChimeraTK::DataType("int32")}},
      NumericAddressedRegisterInfo::Access::READ_WRITE, numericId);
  BOOST_TEST((channelRegisterWithId.numericInterruptId() == numericId));

  NumericAddressedRegisterInfo scalarRegisterWithId("SCALAR_REGISTER", 1, 0x0, 4, 0, 32, 0, true,
      NumericAddressedRegisterInfo::Access::READ_WRITE, NumericAddressedRegisterInfo::Type::FIXED_POINT, numericId);
  BOOST_TEST((scalarRegisterWithId.numericInterruptId() == numericId));
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
