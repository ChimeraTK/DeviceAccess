// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK

#include "BackendFactory.h"
#include "Device.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"
#include "MapFileParser.h"
#include "TwoDRegisterAccessor.h"

#include <iostream>
#include <sstream>

#define BOOST_TEST_MODULE MultiplexedDataAccessorTest
#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

using namespace ChimeraTK;

static const std::string TEST_MODULE_NAME("TEST");
static const std::string INVALID_MODULE_NAME("INVALID");
static const RegisterPath TEST_MODULE_PATH(TEST_MODULE_NAME);
static const RegisterPath INVALID_MODULE_PATH(INVALID_MODULE_NAME);

struct TestParameters {
  std::string dmapFile;
  std::string deviceAlias;
  std::string deviceInvalidAlias;
  std::string deviceMixedAlias;
  std::string mapFileName;
};

// This is necessary for the data test cases
std::ostream& operator<<(std::ostream& o, const TestParameters& p) {
  o << p.dmapFile << " " << p.deviceAlias << " " << p.deviceInvalidAlias << " " << p.deviceMixedAlias << " "
    << p.mapFileName;

  return o;
}

static TestParameters AREA_PARAMS{"dummies.dmap", "SEQUENCES", "INVALID_SEQUENCES", "SEQUENCES", "sequences.map"};

static TestParameters NEW_AREA_PARAMS{
    "newMuxedDummies.dmap", "NEW_SEQUENCES", "NEW_INVALID_SEQUENCES", "NEW_SEQUENCES", "newSequences.mapp"};

BOOST_AUTO_TEST_SUITE(MultiplexedDataAccessorTestSuite)

BOOST_DATA_TEST_CASE(testConstructor, boost::unit_test::data::make({AREA_PARAMS, NEW_AREA_PARAMS})) {
  BackendFactory::getInstance().setDMapFilePath(sample.dmapFile);
  Device device;
  device.open(sample.deviceAlias);
  TwoDRegisterAccessor<double> deMultiplexer = device.getTwoDRegisterAccessor<double>(TEST_MODULE_PATH / "FRAC_INT");
  BOOST_TEST(deMultiplexer[0].size() == 5);

  device.close();
  BOOST_CHECK_THROW(device.open(sample.deviceInvalidAlias), ChimeraTK::logic_error);
}

// test the de-multiplexing itself, with 'identity' fixed point conversion
template<class SequenceWordType>
void testDeMultiplexing(std::string areaName, const TestParameters& sample) {
  std::cout << "testDeMultiplexing areaName = " << areaName
            << "  SequenceWordType = " << typeid(SequenceWordType).name() << std::endl;

  // open a dummy device with the sequence map file
  BackendFactory::getInstance().setDMapFilePath(sample.dmapFile);
  Device device;
  device.open(sample.deviceAlias);

  std::vector<SequenceWordType> ioBuffer(15);
  auto area = device.getOneDRegisterAccessor<int32_t>(TEST_MODULE_NAME + "/" + areaName + ".MULTIPLEXED_RAW");
  size_t nBytes = std::min(area.getNElements() * sizeof(int32_t), 15 * sizeof(SequenceWordType));
  ioBuffer[0] = 'A';
  ioBuffer[1] = 'a';
  ioBuffer[2] = '0';
  ioBuffer[3] = 'B';
  ioBuffer[4] = 'b';
  ioBuffer[5] = '1';
  ioBuffer[6] = 'C';
  ioBuffer[7] = 'c';
  ioBuffer[8] = '2';
  ioBuffer[9] = 'D';
  ioBuffer[10] = 'd';
  ioBuffer[11] = '3';
  ioBuffer[12] = 'E';
  ioBuffer[13] = 'e';
  ioBuffer[14] = '4';
  memcpy(&(area[0]), ioBuffer.data(), nBytes);
  area.write();

  TwoDRegisterAccessor<SequenceWordType> deMultiplexer =
      device.getTwoDRegisterAccessor<SequenceWordType>(TEST_MODULE_PATH / areaName);

  BOOST_TEST(deMultiplexer.isReadOnly() == false);
  BOOST_TEST(deMultiplexer.isReadable());
  BOOST_TEST(deMultiplexer.isWriteable());

  deMultiplexer.read();

  BOOST_TEST(deMultiplexer[0][0] == 'A');
  BOOST_TEST(deMultiplexer[0][1] == 'B');
  BOOST_TEST(deMultiplexer[0][2] == 'C');
  BOOST_TEST(deMultiplexer[0][3] == 'D');
  BOOST_TEST(deMultiplexer[0][4] == 'E');
  BOOST_TEST(deMultiplexer[1][0] == 'a');
  BOOST_TEST(deMultiplexer[1][1] == 'b');
  BOOST_TEST(deMultiplexer[1][2] == 'c');
  BOOST_TEST(deMultiplexer[1][3] == 'd');
  BOOST_TEST(deMultiplexer[1][4] == 'e');
  BOOST_TEST(deMultiplexer[2][0] == '0');
  BOOST_TEST(deMultiplexer[2][1] == '1');
  BOOST_TEST(deMultiplexer[2][2] == '2');
  BOOST_TEST(deMultiplexer[2][3] == '3');
  BOOST_TEST(deMultiplexer[2][4] == '4');

  for(size_t sequenceIndex = 0; sequenceIndex < 3; ++sequenceIndex) {
    for(size_t i = 0; i < 5; ++i) {
      deMultiplexer[sequenceIndex][i] += 5;
    }
  }

  deMultiplexer.write();
  area.read();
  memcpy(ioBuffer.data(), &(area[0]), nBytes);

  BOOST_TEST(ioBuffer[0] == 'F');
  BOOST_TEST(ioBuffer[1] == 'f');
  BOOST_TEST(ioBuffer[2] == '5');
  BOOST_TEST(ioBuffer[3] == 'G');
  BOOST_TEST(ioBuffer[4] == 'g');
  BOOST_TEST(ioBuffer[5] == '6');
  BOOST_TEST(ioBuffer[6] == 'H');
  BOOST_TEST(ioBuffer[7] == 'h');
  BOOST_TEST(ioBuffer[8] == '7');
  BOOST_TEST(ioBuffer[9] == 'I');
  BOOST_TEST(ioBuffer[10] == 'i');
  BOOST_TEST(ioBuffer[11] == '8');
  BOOST_TEST(ioBuffer[12] == 'J');
  BOOST_TEST(ioBuffer[13] == 'j');
  BOOST_TEST(ioBuffer[14] == '9');
}

BOOST_DATA_TEST_CASE(testDeMultiplexing32, boost::unit_test::data::make({AREA_PARAMS, NEW_AREA_PARAMS})) {
  testDeMultiplexing<int32_t>("INT", sample);
}
BOOST_DATA_TEST_CASE(testDeMultiplexing16, boost::unit_test::data::make({AREA_PARAMS, NEW_AREA_PARAMS})) {
  testDeMultiplexing<int16_t>("SHORT", sample);
}
BOOST_DATA_TEST_CASE(testDeMultiplexing8, boost::unit_test::data::make({AREA_PARAMS, NEW_AREA_PARAMS})) {
  testDeMultiplexing<int8_t>("CHAR", sample);
}

// test the de-multiplexing itself, with  fixed point conversion
// and using the factory function
template<class SequenceWordType>
void testWithConversion(std::string multiplexedSequenceName, const TestParameters& sample) {
  // open a dummy device with the sequence map file
  BackendFactory::getInstance().setDMapFilePath(sample.dmapFile);
  Device device;
  device.open(sample.deviceAlias);

  std::vector<SequenceWordType> ioBuffer(15);
  auto area = device.getOneDRegisterAccessor<int32_t>(TEST_MODULE_PATH / multiplexedSequenceName / "MULTIPLEXED_RAW");
  size_t nBytes = std::min(area.getNElements() * sizeof(int32_t), 15 * sizeof(SequenceWordType));

  for(size_t i = 0; i < ioBuffer.size(); ++i) {
    ioBuffer[i] = i;
  }
  memcpy(&(area[0]), ioBuffer.data(), nBytes);
  area.write();

  TwoDRegisterAccessor<float> accessor =
      device.getTwoDRegisterAccessor<float>(TEST_MODULE_PATH / multiplexedSequenceName);
  accessor.read();

  BOOST_TEST(accessor[0][0] == 0);
  BOOST_TEST(accessor[1][0] == 0.25);  // 1 with 2 frac bits
  BOOST_TEST(accessor[2][0] == 0.25);  // 2 with 3 frac bits
  BOOST_TEST(accessor[0][1] == 1.5);   // 3 with 1 frac bits
  BOOST_TEST(accessor[1][1] == 1);     // 4 with 2 frac bits
  BOOST_TEST(accessor[2][1] == 0.625); // 5 with 3 frac bits
  BOOST_TEST(accessor[0][2] == 3.);    // 6 with 1 frac bits
  BOOST_TEST(accessor[1][2] == 1.75);  // 7 with 2 frac bits
  BOOST_TEST(accessor[2][2] == 1.);    // 8 with 3 frac bits
  BOOST_TEST(accessor[0][3] == 4.5);   // 9 with 1 frac bits
  BOOST_TEST(accessor[1][3] == 2.5);   // 10 with 2 frac bits
  BOOST_TEST(accessor[2][3] == 1.375); // 11 with 3 frac bits
  BOOST_TEST(accessor[0][4] == 6.);    // 12 with 1 frac bits
  BOOST_TEST(accessor[1][4] == 3.25);  // 13 with 2 frac bits
  BOOST_TEST(accessor[2][4] == 1.75);  // 14 with 3 frac bits

  for(size_t sequenceIndex = 0; sequenceIndex < 3; ++sequenceIndex) {
    for(size_t i = 0; i < 5; ++i) {
      accessor[sequenceIndex][i] += 1.;
    }
  }

  accessor.write();

  area.read();
  memcpy(ioBuffer.data(), &(area[0]), nBytes);

  for(size_t i = 0; i < 15; ++i) {
    // with i%3+1 fractional bits the added floating point value of 1
    // corresponds to 2^(i%3+1) in fixed point represetation
    int addedValue = 1 << (i % 3 + 1);
    std::stringstream message;
    message << "ioBuffer[" << i << "] is " << ioBuffer[i] << ", expected " << i + addedValue;
    BOOST_CHECK_MESSAGE(ioBuffer[i] == static_cast<SequenceWordType>(i + addedValue), message.str());
  }
}

BOOST_DATA_TEST_CASE(testWithConversion32, boost::unit_test::data::make({AREA_PARAMS, NEW_AREA_PARAMS})) {
  testWithConversion<int32_t>("FRAC_INT", sample);
}
BOOST_DATA_TEST_CASE(testWithConversion16, boost::unit_test::data::make({AREA_PARAMS, NEW_AREA_PARAMS})) {
  testWithConversion<int16_t>("FRAC_SHORT", sample);
}
BOOST_DATA_TEST_CASE(testWithConversion8, boost::unit_test::data::make({AREA_PARAMS, NEW_AREA_PARAMS})) {
  testWithConversion<int8_t>("FRAC_CHAR", sample);
}

BOOST_DATA_TEST_CASE(testMixed, boost::unit_test::data::make({AREA_PARAMS, NEW_AREA_PARAMS})) {
  // open a dummy device with the sequence map file
  BackendFactory::getInstance().setDMapFilePath(sample.dmapFile);
  Device device;
  device.open(sample.deviceMixedAlias);

  TwoDRegisterAccessor<double> myMixedData = device.getTwoDRegisterAccessor<double>("APP0/DAQ0_BAM");
  auto myRawData = device.getOneDRegisterAccessor<int32_t>("APP0/DAQ0_BAM.MULTIPLEXED_RAW", 0, 0, {AccessMode::raw});

  BOOST_TEST(myMixedData.getNChannels() == 17);
  BOOST_TEST(myMixedData.getNElementsPerChannel() == 372);
  BOOST_TEST(myMixedData[0].size() == 372);

  myMixedData[0][0] = -24673; // 1001 1111 1001 1111
  myMixedData[1][0] = -13724; // 1100 1010 0110 0100
  myMixedData[2][0] = 130495;
  myMixedData[3][0] = 513;
  myMixedData[4][0] = 1027;
  myMixedData[5][0] = -56.4;
  myMixedData[6][0] = 78;
  myMixedData[7][0] = 45.2;
  myMixedData[8][0] = -23.9;
  myMixedData[9][0] = 61.3;
  myMixedData[10][0] = -12;

  myMixedData.write();

  myRawData.read();
  BOOST_TEST(myRawData[0] == -899375201);
  BOOST_TEST(myRawData[1] == 130495);
  BOOST_TEST(myRawData[2] == 67305985);
  BOOST_TEST(myRawData[3] == 5112008);
  BOOST_TEST(myRawData[4] == -197269459);

  myMixedData.read();

  BOOST_TEST(myMixedData[0][0] == -24673);
  BOOST_TEST(myMixedData[1][0] == -13724);
  BOOST_TEST(myMixedData[2][0] == 130495);
  BOOST_TEST(myMixedData[3][0] == 513);
  BOOST_TEST(myMixedData[4][0] == 1027);
  BOOST_TEST(myMixedData[5][0] == -56);
  BOOST_TEST(myMixedData[6][0] == 78);
  BOOST_TEST(myMixedData[7][0] == 45);
  BOOST_TEST(myMixedData[8][0] == -24);
  BOOST_TEST(myMixedData[9][0] == 61);
  BOOST_TEST(myMixedData[10][0] == -12);
}

BOOST_DATA_TEST_CASE(testNumberOfSequencesDetected, boost::unit_test::data::make({AREA_PARAMS, NEW_AREA_PARAMS})) {
  auto registerMap = MapFileParser().parse(sample.mapFileName).first;
  // open a dummy device with the sequence map file
  BackendFactory::getInstance().setDMapFilePath(sample.dmapFile);
  Device device;
  device.open(sample.deviceAlias);

  TwoDRegisterAccessor<double> deMuxedData = device.getTwoDRegisterAccessor<double>(TEST_MODULE_PATH / "FRAC_INT");

  BOOST_TEST(deMuxedData.getNChannels() == 3);
}

BOOST_DATA_TEST_CASE(testAreaOfInterestOffset, boost::unit_test::data::make({AREA_PARAMS, NEW_AREA_PARAMS})) {
  // open a dummy device with the sequence map file
  BackendFactory::getInstance().setDMapFilePath(sample.dmapFile);
  Device device;
  device.open(sample.deviceMixedAlias);

  // There are 44 bytes per block. In total the area is 4096 bytes long
  // => There are 372 elements (=4092 bytes) in the area, the last 4 bytes are unused.
  const size_t nWordsPerBlock = 44 / sizeof(int32_t); // 32 bit raw words on the transport layer

  // we only request 300 of the 372 elements, with 42 elements offset, so we just cut out an area in the middle
  TwoDRegisterAccessor<double> myMixedData = device.getTwoDRegisterAccessor<double>("APP0/DAQ0_BAM", 300, 42);
  auto myRawData = device.getOneDRegisterAccessor<int32_t>(
      "APP0/DAQ0_BAM.MULTIPLEXED_RAW", 300 * nWordsPerBlock, 42 * nWordsPerBlock, {AccessMode::raw});

  BOOST_TEST(myMixedData.getNChannels() == 17);
  BOOST_TEST(myMixedData.getNElementsPerChannel() == 300);
  BOOST_TEST(myMixedData[0].size() == 300);

  for(unsigned int i = 0; i < myMixedData.getNElementsPerChannel(); ++i) {
    myMixedData[0][i] = -24673; // 1001 1111 1001 1111
    myMixedData[1][i] = -13724; // 1100 1010 0110 0100
    myMixedData[2][i] = 130495;
    myMixedData[3][i] = 513;
    myMixedData[4][i] = 1027;
    myMixedData[5][i] = -56.4;
    myMixedData[6][i] = 78;
    myMixedData[7][i] = 45.2;
    myMixedData[8][i] = -23.9;
    myMixedData[9][i] = 61.3;
    myMixedData[10][i] = -12;

    myMixedData.write();

    myRawData.read();
    BOOST_TEST(myRawData[0 + i * nWordsPerBlock] == -899375201);
    BOOST_TEST(myRawData[1 + i * nWordsPerBlock] == 130495);
    BOOST_TEST(myRawData[2 + i * nWordsPerBlock] == 67305985);
    BOOST_TEST(myRawData[3 + i * nWordsPerBlock] == 5112008);
    BOOST_TEST(myRawData[4 + i * nWordsPerBlock] == -197269459);

    myMixedData.read();

    BOOST_TEST(myMixedData[0][i] == -24673);
    BOOST_TEST(myMixedData[1][i] == -13724);
    BOOST_TEST(myMixedData[2][i] == 130495);
    BOOST_TEST(myMixedData[3][i] == 513);
    BOOST_TEST(myMixedData[4][i] == 1027);
    BOOST_TEST(myMixedData[5][i] == -56);
    BOOST_TEST(myMixedData[6][i] == 78);
    BOOST_TEST(myMixedData[7][i] == 45);
    BOOST_TEST(myMixedData[8][i] == -24);
    BOOST_TEST(myMixedData[9][i] == 61);
    BOOST_TEST(myMixedData[10][i] == -12);

    myMixedData[0][i] = i;
    myMixedData[1][i] = 0;
    myMixedData[2][i] = 0;
    myMixedData[3][i] = 0;
    myMixedData[4][i] = 0;
    myMixedData[5][i] = 0;
    myMixedData[6][i] = 0;
    myMixedData[7][i] = 0;
    myMixedData[8][i] = 0;
    myMixedData[9][i] = 0;
    myMixedData[10][i] = 0;

    myMixedData.write();

    myRawData.read();
    BOOST_TEST(myRawData[0 + i * nWordsPerBlock] == static_cast<int>(i));
    BOOST_TEST(myRawData[1 + i * nWordsPerBlock] == 0);
    BOOST_TEST(myRawData[2 + i * nWordsPerBlock] == 0);
    BOOST_TEST(myRawData[3 + i * nWordsPerBlock] == 0);
    BOOST_TEST(myRawData[4 + i * nWordsPerBlock] == 0);

    myMixedData.read();

    BOOST_TEST(myMixedData[0][i] == i);
    BOOST_TEST(myMixedData[1][i] == 0);
    BOOST_TEST(myMixedData[2][i] == 0);
    BOOST_TEST(myMixedData[3][i] == 0);
    BOOST_TEST(myMixedData[4][i] == 0);
    BOOST_TEST(myMixedData[5][i] == 0);
    BOOST_TEST(myMixedData[6][i] == 0);
    BOOST_TEST(myMixedData[7][i] == 0);
    BOOST_TEST(myMixedData[8][i] == 0);
    BOOST_TEST(myMixedData[9][i] == 0);
    BOOST_TEST(myMixedData[10][i] == 0);
  }
}

BOOST_DATA_TEST_CASE(testAreaOfInterestLength, boost::unit_test::data::make({AREA_PARAMS, NEW_AREA_PARAMS})) {
  // open a dummy device with the sequence map file
  BackendFactory::getInstance().setDMapFilePath(sample.dmapFile);
  Device device;
  device.open(sample.deviceMixedAlias);

  const size_t nWordsPerBlock = 44 / sizeof(int32_t);

  TwoDRegisterAccessor<double> myMixedData = device.getTwoDRegisterAccessor<double>("APP0/DAQ0_BAM", 120);
  auto myRawData = device.getOneDRegisterAccessor<int32_t>("APP0/DAQ0_BAM.MULTIPLEXED_RAW", 0, 0, {AccessMode::raw});

  BOOST_TEST(myMixedData.getNChannels() == 17);
  BOOST_TEST(myMixedData.getNElementsPerChannel() == 120);
  BOOST_TEST(myMixedData[0].size() == 120);

  for(unsigned int i = 0; i < myMixedData.getNElementsPerChannel(); ++i) {
    myMixedData[0][i] = -24673; // 1001 1111 1001 1111
    myMixedData[1][i] = -13724; // 1100 1010 0110 0100
    myMixedData[2][i] = 130495;
    myMixedData[3][i] = 513;
    myMixedData[4][i] = 1027;
    myMixedData[5][i] = -56.4;
    myMixedData[6][i] = 78;
    myMixedData[7][i] = 45.2;
    myMixedData[8][i] = -23.9;
    myMixedData[9][i] = 61.3;
    myMixedData[10][i] = -12;

    myMixedData.write();

    myRawData.read();
    BOOST_TEST(myRawData[0 + i * nWordsPerBlock] == -899375201);
    BOOST_TEST(myRawData[1 + i * nWordsPerBlock] == 130495);
    BOOST_TEST(myRawData[2 + i * nWordsPerBlock] == 67305985);
    BOOST_TEST(myRawData[3 + i * nWordsPerBlock] == 5112008);
    BOOST_TEST(myRawData[4 + i * nWordsPerBlock] == -197269459);

    myMixedData.read();

    BOOST_TEST(myMixedData[0][i] == -24673);
    BOOST_TEST(myMixedData[1][i] == -13724);
    BOOST_TEST(myMixedData[2][i] == 130495);
    BOOST_TEST(myMixedData[3][i] == 513);
    BOOST_TEST(myMixedData[4][i] == 1027);
    BOOST_TEST(myMixedData[5][i] == -56);
    BOOST_TEST(myMixedData[6][i] == 78);
    BOOST_TEST(myMixedData[7][i] == 45);
    BOOST_TEST(myMixedData[8][i] == -24);
    BOOST_TEST(myMixedData[9][i] == 61);
    BOOST_TEST(myMixedData[10][i] == -12);

    myMixedData[0][i] = i;
    myMixedData[1][i] = 0;
    myMixedData[2][i] = 0;
    myMixedData[3][i] = 0;
    myMixedData[4][i] = 0;
    myMixedData[5][i] = 0;
    myMixedData[6][i] = 0;
    myMixedData[7][i] = 0;
    myMixedData[8][i] = 0;
    myMixedData[9][i] = 0;
    myMixedData[10][i] = 0;

    myMixedData.write();

    myRawData.read();
    BOOST_TEST(myRawData[0 + i * nWordsPerBlock] == (int)i);
    BOOST_TEST(myRawData[1 + i * nWordsPerBlock] == 0);
    BOOST_TEST(myRawData[2 + i * nWordsPerBlock] == 0);
    BOOST_TEST(myRawData[3 + i * nWordsPerBlock] == 0);
    BOOST_TEST(myRawData[4 + i * nWordsPerBlock] == 0);

    myMixedData.read();

    BOOST_TEST(myMixedData[0][i] == i);
    BOOST_TEST(myMixedData[1][i] == 0);
    BOOST_TEST(myMixedData[2][i] == 0);
    BOOST_TEST(myMixedData[3][i] == 0);
    BOOST_TEST(myMixedData[4][i] == 0);
    BOOST_TEST(myMixedData[5][i] == 0);
    BOOST_TEST(myMixedData[6][i] == 0);
    BOOST_TEST(myMixedData[7][i] == 0);
    BOOST_TEST(myMixedData[8][i] == 0);
    BOOST_TEST(myMixedData[9][i] == 0);
    BOOST_TEST(myMixedData[10][i] == 0);
  }
}

/**********************************************************************************************************************/

// Runtime 'selectedBy' gating on polled 2D muxed registers (plan tests PD1-PD2).
//
// simpleJsonFile.jmap's DAQ.MUXED_WITH_STATUS is a muxed 2D register with per-channel selections: AmplitudeCh0 and
// PhaseCh0 are active while DAQ.MUX_SEL == 0, StatusCh0 and StatusCh1 while DAQ.MUX_SEL == 2. Because the framework
// has no per-channel DataValidity (dataValidity() is accessor-global) and the muxed per-channel gating collapses to
// a global "faulty if any channel inactive" signal on the full register, these tests assert the gate on the
// single-channel slice accessors, where the accessor-global validity equals that channel's own selection state.
BOOST_AUTO_TEST_CASE(testSelectedByMuxedChannelSlices) {
  Device device;
  device.open("(dummy?map=simpleJsonFile.jmap)");
  auto dummy = boost::dynamic_pointer_cast<DummyBackend>(device.getBackend());
  BOOST_REQUIRE(dummy != nullptr);

  DummyRegisterAccessor<uint32_t> muxSel(dummy.get(), "DAQ", "MUX_SEL");
  DummyRegisterAccessor<int16_t> amp0(dummy.get(), "DAQ/MUXED_WITH_STATUS", "AmplitudeCh0");
  DummyRegisterAccessor<int16_t> status0(dummy.get(), "DAQ/MUXED_WITH_STATUS", "StatusCh0");

  auto amp0Acc = device.getOneDRegisterAccessor<int16_t>("/DAQ/MUXED_WITH_STATUS/AmplitudeCh0");
  auto status0Acc = device.getOneDRegisterAccessor<int16_t>("/DAQ/MUXED_WITH_STATUS/StatusCh0");

  // PD1: select DAQ.MUX_SEL == 0 -> the AmplitudeCh0 channel is active and delivers valid data.
  muxSel[0] = 0;
  amp0[0] = 321;
  amp0Acc.read();
  BOOST_CHECK(amp0Acc.dataValidity() == ChimeraTK::DataValidity::ok);

  // The StatusCh0 channel (selected by MUX_SEL == 2) is inactive while MUX_SEL == 0.
  status0Acc.read();
  BOOST_CHECK(status0Acc.dataValidity() == ChimeraTK::DataValidity::faulty);

  // PD2: select DAQ.MUX_SEL == 2 -> StatusCh0 becomes active, AmplitudeCh0 becomes faulty.
  muxSel[0] = 2;
  status0[0] = 654;
  status0Acc.read();
  BOOST_CHECK(status0Acc.dataValidity() == ChimeraTK::DataValidity::ok);

  amp0Acc.read();
  BOOST_CHECK(amp0Acc.dataValidity() == ChimeraTK::DataValidity::faulty);

  device.close();
}

/**********************************************************************************************************************/

// PD3: per-channel gate on a full 2D read — the active channel set of a muxed register is visible in a single
// read via getDataValidityOfChannels(): the selected channel is ok while the unselected partner of the same
// read is faulty (plan: "the selected channels of the same read remain valid"). Uses MQ.FD (2D DMA, Ch0 sel 0,
// Ch1 sel 1, uniform 16-bit channels) so the whole register reads cleanly as int16_t.
BOOST_AUTO_TEST_CASE(testSelectedByMuxedPerChannelValidity) {
  Device device;
  device.open("(dummy?map=muxedPolled.jmap)");

  DummyRegisterAccessor<int32_t> mux(
      boost::dynamic_pointer_cast<DummyBackend>(device.getBackend()).get(), "MQ", "MUX");
  auto fd = device.getBackend()->getRegisterAccessor<int16_t>("/MQ/FD", 0, 0, {});
  BOOST_REQUIRE(fd->getNumberOfChannels() == 2);

  // Select Ch0 (MUX == 0): Ch0 valid, Ch1 faulty in the same read.
  mux[0] = 0;
  fd->read();
  BOOST_REQUIRE(fd->getDataValidityOfChannels().size() == 2);
  BOOST_CHECK(fd->getDataValidityOfChannels()[0] == ChimeraTK::DataValidity::ok);
  BOOST_CHECK(fd->getDataValidityOfChannels()[1] == ChimeraTK::DataValidity::faulty);

  // Select Ch1 (MUX == 1): Ch1 valid, Ch0 faulty.
  mux[0] = 1;
  fd->read();
  BOOST_CHECK(fd->getDataValidityOfChannels()[0] == ChimeraTK::DataValidity::faulty);
  BOOST_CHECK(fd->getDataValidityOfChannels()[1] == ChimeraTK::DataValidity::ok);

  device.close();
}

BOOST_AUTO_TEST_SUITE_END()
