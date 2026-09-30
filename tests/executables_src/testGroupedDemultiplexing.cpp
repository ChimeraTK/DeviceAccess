// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE testGroupedDemultiplexing
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"
#include "Exception.h"
#include "MuxedChannelDemultiplexer.h"
#include "TransferGroup.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <typeindex>
#include <vector>

using namespace ChimeraTK;

/**********************************************************************************************************************/

constexpr size_t nSamples = 4;

/// Open the ExceptionDummy backend on the map carrying the 2D registers exercised here (NODMA, MIXED, DBL, DBLASYNC).
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
// Read all channel slices of a muxed 2D register in one TransferGroup (including one that is not first in memory).

BOOST_AUTO_TEST_CASE(TestGroupedReadAllChannels) {
  auto device = openDevice();
  writeNodma(device, 2000);

  std::vector<ChimeraTK::OneDRegisterAccessor<int16_t>> accessors;
  TransferGroup group;
  for(int c = 0; c < 16; ++c) {
    auto a = device.getOneDRegisterAccessor<int16_t>("/TEST/NODMA." + std::to_string(c));
    accessors.push_back(a);
    group.addAccessor(a);
  }
  group.read();

  for(int c = 0; c < 16; ++c) {
    for(size_t e = 0; e < nSamples; ++e) {
      BOOST_TEST(accessors[c][e] == static_cast<int16_t>(2000 + c * 10 + e));
    }
  }
  device.close();
}

/**********************************************************************************************************************/
// Cover a partial subset of the channels (not all) in one group.

BOOST_AUTO_TEST_CASE(TestGroupedReadPartialSubset) {
  auto device = openDevice();
  writeNodma(device, 7000);

  auto ch2 = device.getOneDRegisterAccessor<int16_t>("/TEST/NODMA.2");
  auto ch7 = device.getOneDRegisterAccessor<int16_t>("/TEST/NODMA.7");
  auto ch9 = device.getOneDRegisterAccessor<int16_t>("/TEST/NODMA.9");

  TransferGroup group;
  group.addAccessor(ch9);
  group.addAccessor(ch2);
  group.addAccessor(ch7);
  group.read();

  for(size_t e = 0; e < nSamples; ++e) {
    BOOST_TEST(ch2[e] == static_cast<int16_t>(7000 + 2 * 10 + e));
    BOOST_TEST(ch7[e] == static_cast<int16_t>(7000 + 7 * 10 + e));
    BOOST_TEST(ch9[e] == static_cast<int16_t>(7000 + 9 * 10 + e));
  }
  device.close();
}

/**********************************************************************************************************************/
// Mixed user types and conversion parameters, so several groups form within one read. All channels share one raw
// element (MIXED lives in the 2D area at AREA byte offset 0x100).

BOOST_AUTO_TEST_CASE(TestMixedConversionParamsAndTypes) {
  auto device = openDevice();

  // Write the MIXED area with exact byte control through the raw AREA accessor. Channel c sample s sits in 4-byte word
  // (64 + c + s*8): channel offset (0/4/8/12) in the area at word 64, pitch 32 bytes. Each 2-byte value sits in the low
  // two bytes of the word.
  auto area = device.getOneDRegisterAccessor<int32_t>("/AREA_DMAABLE", 0, 0, {AccessMode::raw});
  std::array<std::array<int16_t, 4>, 4> vals = {
      {{100, 101, 102, 103}, {200, 201, 202, 203}, {30, 31, 32, 33}, {400, 401, 402, 403}}};
  for(int c = 0; c < 4; ++c) {
    for(int s = 0; s < 4; ++s) {
      int32_t word = 0;
      std::memcpy(&word, &vals[c][s], 2);
      area[64 + c + s * 8] = word;
    }
  }
  area.write();

  // Distinct conversion parameters / user types, so they cannot share one raw conversion group:
  //   0: signed fixed point, width 16          (int16_t)
  //   1: unsigned fixed point, width 16        (uint16_t)
  //   2: signed fixed point, width 12          (int16_t)
  //   3: signed fixed point, width 16, frac 4  (int16_t)
  auto ch0 = device.getOneDRegisterAccessor<int16_t>("/TEST/MIXED.0");
  auto ch1 = device.getOneDRegisterAccessor<uint16_t>("/TEST/MIXED.1");
  auto ch2 = device.getOneDRegisterAccessor<int16_t>("/TEST/MIXED.2");
  auto ch3 = device.getOneDRegisterAccessor<int16_t>("/TEST/MIXED.3");

  TransferGroup group;
  group.addAccessor(ch0);
  group.addAccessor(ch1);
  group.addAccessor(ch2);
  group.addAccessor(ch3);
  group.read();

  for(size_t e = 0; e < nSamples; ++e) {
    BOOST_TEST(ch0[e] == 100 + e);
    BOOST_TEST(ch1[e] == 200 + e);
    BOOST_TEST(ch2[e] == 30 + e);
    // channel 3 has 4 fractional bits, so 400..403 are delivered scaled by 2^-4
    BOOST_TEST(ch3[e] == (400 + e) / 16);
  }

  // All slices served by the shared raw element deliver the same version number and valid data.
  BOOST_TEST(ch0.getVersionNumber() == ch1.getVersionNumber());
  BOOST_TEST(ch1.getVersionNumber() == ch2.getVersionNumber());
  BOOST_TEST(ch2.getVersionNumber() == ch3.getVersionNumber());
  BOOST_CHECK(ch0.dataValidity() == ChimeraTK::DataValidity::ok);
  BOOST_CHECK(ch1.dataValidity() == ChimeraTK::DataValidity::ok);
  BOOST_CHECK(ch2.dataValidity() == ChimeraTK::DataValidity::ok);
  BOOST_CHECK(ch3.dataValidity() == ChimeraTK::DataValidity::ok);
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
// Per-slice version number and data validity match the shared element, and writes through the 2D accessor show up in
// the grouped channel read.

BOOST_AUTO_TEST_CASE(TestVersionNumberAndDataValidity) {
  auto device = openDevice();
  writeNodma(device, 4000);

  auto ch0 = device.getOneDRegisterAccessor<int16_t>("/TEST/NODMA.0");
  auto ch5 = device.getOneDRegisterAccessor<int16_t>("/TEST/NODMA.5");
  auto ch12 = device.getOneDRegisterAccessor<int16_t>("/TEST/NODMA.12");

  TransferGroup group;
  group.addAccessor(ch12);
  group.addAccessor(ch0);
  group.addAccessor(ch5);
  group.read();

  // All slices share one low-level element, hence one version number.
  BOOST_TEST(ch0.getVersionNumber() == ch5.getVersionNumber());
  BOOST_TEST(ch5.getVersionNumber() == ch12.getVersionNumber());
  for(auto* ch : {&ch0, &ch5, &ch12}) {
    BOOST_REQUIRE(ch->dataValidity() == ChimeraTK::DataValidity::ok);
  }

  // A second read of the group bumps the shared version and keeps it identical across slices.
  auto firstVersion = ch0.getVersionNumber();
  group.read();
  BOOST_TEST(ch0.getVersionNumber() != firstVersion);
  BOOST_TEST(ch0.getVersionNumber() == ch5.getVersionNumber());
  BOOST_TEST(ch0.getVersionNumber() == ch12.getVersionNumber());
  device.close();
}

/**********************************************************************************************************************/
// The demultiplexing flag is set on fresh data and cleared by run(), so the conversion runs at most once per
// transfer-group read even when several consumers trigger post-read.

BOOST_AUTO_TEST_CASE(TestDemultiplexerFlagOncePerRead) {
  using detail::MuxedChannelDemultiplexer;
  using DataType = NumericAddressedRegisterInfo::Type;
  MuxedChannelDemultiplexer demux;
  BOOST_TEST(demux.empty());
  BOOST_TEST(!demux.hasNewData());

  // run() safely clears the flag with no consumer registered.
  demux.rememberNewData();
  BOOST_TEST(demux.hasNewData());
  demux.run(nullptr);
  BOOST_TEST(!demux.hasNewData());

  MuxedChannelDemultiplexer::GroupKey key{
      DataType::FIXED_POINT, 16, 0, true, ChimeraTK::DataType::int16, std::type_index(typeid(int16_t)), true};
  std::vector<int16_t> cooked0(nSamples, 0);
  std::vector<int16_t> cooked1(nSamples, 0);
  std::array<uint8_t, 64> rawBuffer;

  MuxedChannelDemultiplexer::Consumer consumer0{20, 8, nSamples,
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      [&]() { return reinterpret_cast<uint8_t*>(cooked0.data()); }};
  MuxedChannelDemultiplexer::Consumer consumer1{30, 8, nSamples,
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      [&]() { return reinterpret_cast<uint8_t*>(cooked1.data()); }};
  // A Registration handle keeps the consumer registered until destroyed.
  MuxedChannelDemultiplexer::Registration<int16_t> reg0(&demux, key, consumer0);
  MuxedChannelDemultiplexer::Registration<int16_t> reg1(&demux, key, consumer1);
  BOOST_TEST(!demux.empty());

  // Repeated post-read entries must run the demultiplexing only once.
  demux.rememberNewData();
  BOOST_TEST(demux.hasNewData());
  demux.run(rawBuffer.data());
  BOOST_TEST(!demux.hasNewData());

  // A second run() with no fresh data is a no-op.
  demux.run(rawBuffer.data());
  BOOST_TEST(!demux.hasNewData());
}

/**********************************************************************************************************************/
// Interrupt-driven double-buffered register: a fresh buffer is delivered through the demultiplexer path with correct
// data and validity. wait_for_new_data slices cannot be combined in a TransferGroup, so this reads a single strided
// slice.

BOOST_AUTO_TEST_CASE(TestInterruptDrivenDoubleBufferSlice) {
  auto device = openDevice();
  auto backend = boost::dynamic_pointer_cast<ChimeraTK::DummyBackend>(device.getBackend());
  BOOST_REQUIRE(backend);

  DummyRegisterAccessor<int16_t> buffer0{backend.get(), "TEST/DBLASYNC.1", "BUF0"};
  DummyRegisterAccessor<int16_t> buffer1{backend.get(), "TEST/DBLASYNC.1", "BUF1"};
  DummyRegisterAccessor<uint32_t> inactive{backend.get(), "TEST/DOUBLE_BUF", "INACTIVE_BUF_ID"};

  auto ch1 = device.getOneDRegisterAccessor<int16_t>("/TEST/DBLASYNC.1", 0, 0, {AccessMode::wait_for_new_data});
  device.activateAsyncRead();

  // An initial value is delivered when the async domain is activated.
  BOOST_REQUIRE(ch1.readNonBlocking());
  BOOST_REQUIRE(ch1.dataValidity() == ChimeraTK::DataValidity::ok);
  auto initialVersion = ch1.getVersionNumber();

  // The firmware finishes buffer 0 and points the inactive-buffer id at it.
  inactive[0] = 1;
  for(size_t e = 0; e < nSamples; ++e) {
    buffer0[e] = static_cast<int16_t>(200 + e);
  }
  backend->triggerInterrupt(7);
  BOOST_REQUIRE(ch1.readNonBlocking());

  // A newly read value must carry a new version number and the demultiplexed
  // content of the freshly finished buffer.
  BOOST_TEST(ch1.getVersionNumber() != initialVersion);
  for(size_t e = 0; e < nSamples; ++e) {
    BOOST_TEST(ch1[e] == static_cast<int16_t>(200 + e));
  }
  BOOST_REQUIRE(ch1.dataValidity() == ChimeraTK::DataValidity::ok);
  device.close();
}

/**********************************************************************************************************************/
// Two strided slices of a double-buffered (non-interrupt) register share one raw element and are demultiplexed
// together.

BOOST_AUTO_TEST_CASE(TestDoubleBufferedGroupedRead) {
  auto device = openDevice();
  auto backend = boost::dynamic_pointer_cast<ChimeraTK::DummyBackend>(device.getBackend());
  BOOST_REQUIRE(backend);

  DummyRegisterAccessor<int16_t> buffer0c0{backend.get(), "TEST/DBL.0", "BUF0"};
  DummyRegisterAccessor<int16_t> buffer0c1{backend.get(), "TEST/DBL.1", "BUF0"};
  DummyRegisterAccessor<uint32_t> inactive{backend.get(), "TEST/DOUBLE_BUF", "INACTIVE_BUF_ID"};

  // Finish buffer 0 (inactive-buffer id 1 selects BUF0), then read both slices through one group.
  inactive[0] = 1;
  for(size_t e = 0; e < nSamples; ++e) {
    buffer0c0[e] = static_cast<int16_t>(500 + e);
    buffer0c1[e] = static_cast<int16_t>(600 + e);
  }

  auto ch0 = device.getOneDRegisterAccessor<int16_t>("/TEST/DBL.0");
  auto ch1 = device.getOneDRegisterAccessor<int16_t>("/TEST/DBL.1");
  TransferGroup group;
  group.addAccessor(ch1);
  group.addAccessor(ch0);
  group.read();

  for(size_t e = 0; e < nSamples; ++e) {
    BOOST_TEST(ch0[e] == static_cast<int16_t>(500 + e));
    BOOST_TEST(ch1[e] == static_cast<int16_t>(600 + e));
  }
  device.close();
}

/**********************************************************************************************************************/
// A runtime error during the group read must leave the buffers, version numbers and validity of all slices unchanged.

BOOST_AUTO_TEST_CASE(TestExceptionPathSlicesUnchanged) {
  auto device = openDevice();
  auto backend = boost::dynamic_pointer_cast<ChimeraTK::DummyBackend>(device.getBackend());
  BOOST_REQUIRE(backend);
  writeNodma(device, 3000);

  auto ch0 = device.getOneDRegisterAccessor<int16_t>("/TEST/NODMA.0");
  auto ch5 = device.getOneDRegisterAccessor<int16_t>("/TEST/NODMA.5");
  auto ch12 = device.getOneDRegisterAccessor<int16_t>("/TEST/NODMA.12");

  TransferGroup group;
  group.addAccessor(ch12);
  group.addAccessor(ch0);
  group.addAccessor(ch5);

  // One successful read establishes the reference buffers and version number.
  group.read();
  const auto version = ch0.getVersionNumber();
  for(size_t e = 0; e < nSamples; ++e) {
    BOOST_TEST(ch0[e] == static_cast<int16_t>(3000 + e));
    BOOST_TEST(ch5[e] == static_cast<int16_t>(3000 + 50 + e));
    BOOST_TEST(ch12[e] == static_cast<int16_t>(3000 + 120 + e));
  }
  BOOST_REQUIRE(ch0.dataValidity() == ChimeraTK::DataValidity::ok);

  // A runtime error in the raw read must leave every slice unchanged: the staging buffers were never swapped into the
  // application buffers.
  backend->throwExceptionRead = true;
  BOOST_CHECK_THROW(group.read(), ChimeraTK::runtime_error);
  backend->throwExceptionRead = false;

  for(size_t e = 0; e < nSamples; ++e) {
    BOOST_TEST(ch0[e] == static_cast<int16_t>(3000 + e));
    BOOST_TEST(ch5[e] == static_cast<int16_t>(3000 + 50 + e));
    BOOST_TEST(ch12[e] == static_cast<int16_t>(3000 + 120 + e));
  }
  BOOST_TEST(ch0.getVersionNumber() == version);
  BOOST_TEST(ch5.getVersionNumber() == version);
  BOOST_TEST(ch12.getVersionNumber() == version);
  BOOST_REQUIRE(ch0.dataValidity() == ChimeraTK::DataValidity::ok);
  device.close();
}

/**********************************************************************************************************************/
// Slices of two adjacent muxed 2D registers merge into one transfer element whose raw buffer covers both registers;
// each slice is demultiplexed from its own byte offset within that merged element.

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
