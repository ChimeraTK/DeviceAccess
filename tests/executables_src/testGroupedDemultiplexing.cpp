// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE testGroupedDemultiplexing
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "DummyBackend.h"
#include "Exception.h"
#include "TransferGroup.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

using namespace ChimeraTK;

/**********************************************************************************************************************/

constexpr size_t nSamples = 4;

/// Open the ExceptionDummy backend on the map carrying the 2D registers exercised here (NODMA, NODMA2, MIXED,
/// DBL, DBLASYNC).
static ChimeraTK::Device openDevice() {
  ChimeraTK::Device device;
  device.open("(ExceptionDummy:1?map=groupedDemux.jmap)");
  return device;
}

/// Write per-channel sample values into the given 2D register, so the channel slices can be read back.
static void writeTwoD(ChimeraTK::Device& device, const std::string& name, int16_t base) {
  auto full = device.getTwoDRegisterAccessor<int16_t>(name);
  for(int c = 0; c < 16; ++c) {
    for(size_t e = 0; e < nSamples; ++e) {
      full[c][e] = static_cast<int16_t>(base + c * 10 + e);
    }
  }
  full.write();
}

/// Write per-channel sample values into the 2D register NODMA, so the channel slices can be read back.
static void writeNodma(ChimeraTK::Device& device, int16_t base) {
  writeTwoD(device, "/TEST/NODMA", base);
}

/// Write per-channel sample values into the 2D register NODMA2, directly following NODMA, so its slices can be
/// read back alongside NODMA's.
static void writeNodma2(ChimeraTK::Device& device, int16_t base) {
  writeTwoD(device, "/TEST/NODMA2", base);
}

/**********************************************************************************************************************/
// Create a test suite which holds all the tests.
BOOST_AUTO_TEST_SUITE(GroupedDemultiplexingTestSuite)

/**********************************************************************************************************************/
// Read all channel slices of a muxed 2D register in one TransferGroup, added in shuffled (non-contiguous, out-of-order)
// order so the group members are neither contiguous nor in memory order. Channels 0 and 3 share one conversion key and
// form one multi-channel group, while channels 1 and 2 are differently keyed, so several groups form within the single
// shared element. MIXED lives in the 2D area at AREA byte offset 0x100.

BOOST_AUTO_TEST_CASE(TestGroupedReadAllChannels) {
  auto device = openDevice();

  // Write the MIXED area with exact byte control through the raw AREA accessor. Channel c sample s sits in 4-byte word
  // (64 + c + s*8): channel offset (0/4/8/12) in the area at word 64, pitch 32 bytes. Each 2-byte value sits in the low
  // two bytes of the word.
  auto area = device.getOneDRegisterAccessor<int32_t>("/AREA_DMAABLE", 0, 0, {AccessMode::raw});
  const std::array<std::array<int16_t, nSamples>, 4> vals = {
      {{100, 101, 102, 103}, {200, 201, 202, 203}, {30, 31, 32, 33}, {400, 401, 402, 403}}};
  for(int c = 0; c < 4; ++c) {
    for(size_t s = 0; s < nSamples; ++s) {
      int32_t word = 0;
      std::memcpy(&word, &vals[c][s], 2);
      area[64 + c + s * 8] = word;
    }
  }
  area.write();

  // Distinct conversion parameters / user types, so they cannot all share one raw conversion group:
  //   0: signed fixed point, width 16          (int16_t)  - one multi-channel group with channel 3
  //   3: signed fixed point, width 16          (int16_t)  - one multi-channel group with channel 0
  //   1: unsigned fixed point, width 16        (uint16_t) - differently keyed
  //   2: signed fixed point, width 12          (int16_t)  - differently keyed
  const std::vector<int> channels = {3, 1, 0, 2};
  std::vector<ChimeraTK::OneDRegisterAccessor<int16_t>> accessors;
  TransferGroup group;
  for(int c : channels) {
    auto a = device.getOneDRegisterAccessor<int16_t>("/TEST/MIXED." + std::to_string(c));
    accessors.push_back(a);
    group.addAccessor(a);
  }
  group.read();

  for(size_t i = 0; i < channels.size(); ++i) {
    const int c = channels[i];
    for(size_t e = 0; e < nSamples; ++e) {
      BOOST_TEST(accessors[i][e] == vals[c][e]);
    }
  }

  // All slices served by the shared raw element deliver the same version number and valid data.
  for(const auto& accessor : accessors) {
    BOOST_TEST(accessor.getVersionNumber() == accessors.front().getVersionNumber());
    BOOST_TEST_REQUIRE(accessor.dataValidity() == ChimeraTK::DataValidity::ok);
  }
  device.close();
}

/**********************************************************************************************************************/
// Raw-mode slices are grouped by their user type and copied without conversion.

BOOST_AUTO_TEST_CASE(TestRawModeSlices) {
  auto device = openDevice();
  writeNodma(device, 9000);

  auto r0 = device.getOneDRegisterAccessor<int16_t>("/TEST/NODMA.0", 0, 0, {AccessMode::raw});
  auto r5 = device.getOneDRegisterAccessor<int16_t>("/TEST/NODMA.5", 0, 0, {AccessMode::raw});
  auto r12 = device.getOneDRegisterAccessor<int16_t>("/TEST/NODMA.12", 0, 0, {AccessMode::raw});

  // Slices of a non-interrupt 2D register are read-only.
  BOOST_TEST(!r0.isWriteable());

  TransferGroup group;
  group.addAccessor(r12);
  group.addAccessor(r0);
  group.addAccessor(r5);
  group.read();

  for(size_t e = 0; e < nSamples; ++e) {
    BOOST_TEST(r0[e] == static_cast<int16_t>(9000 + 0 * 10 + e));
    BOOST_TEST(r5[e] == static_cast<int16_t>(9000 + 5 * 10 + e));
    BOOST_TEST(r12[e] == static_cast<int16_t>(9000 + 12 * 10 + e));
  }
  device.close();
}

/**********************************************************************************************************************/
// Slices of two adjacent muxed 2D registers are grouped into one TransferGroup; their channels merge into one shared
// low-level transfer element whose raw buffer covers both registers, and each slice is demultiplexed from its own byte
// offset within that merged element.

BOOST_AUTO_TEST_CASE(TestAdjacentRegistersShareMergedElement) {
  auto device = openDevice();
  writeNodma(device, 2000);
  writeNodma2(device, 4000);

  auto ch0 = device.getOneDRegisterAccessor<int16_t>("/TEST/NODMA.1");
  auto ch4 = device.getOneDRegisterAccessor<int16_t>("/TEST/NODMA.6");
  auto chA = device.getOneDRegisterAccessor<int16_t>("/TEST/NODMA2.0");
  auto chB = device.getOneDRegisterAccessor<int16_t>("/TEST/NODMA2.9");
  auto chC = device.getOneDRegisterAccessor<int16_t>("/TEST/NODMA.15");

  TransferGroup group;
  group.addAccessor(chC);
  group.addAccessor(chA);
  group.addAccessor(ch4);
  group.addAccessor(chB);
  group.addAccessor(ch0);
  group.read();

  // Each slice's byte offset is relative to the single raw buffer covering NODMA and NODMA2.
  for(size_t e = 0; e < nSamples; ++e) {
    BOOST_TEST(ch0[e] == static_cast<int16_t>(2000 + 10 + e));
    BOOST_TEST(ch4[e] == static_cast<int16_t>(2000 + 60 + e));
    BOOST_TEST(chA[e] == static_cast<int16_t>(4000 + e));
    BOOST_TEST(chB[e] == static_cast<int16_t>(4000 + 90 + e));
    BOOST_TEST(chC[e] == static_cast<int16_t>(2000 + 150 + e));
  }

  // Slices of both adjacent registers must share one low-level element.
  auto sharedElement = ch0.getHardwareAccessingElements().front();
  for(auto* accessor : {&ch4, &chA, &chB, &chC}) {
    BOOST_TEST(accessor->getHardwareAccessingElements().front().get() == sharedElement.get());
  }
  device.close();
}

BOOST_AUTO_TEST_SUITE_END()
