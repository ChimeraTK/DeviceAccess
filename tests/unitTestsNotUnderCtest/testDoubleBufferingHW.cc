// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SubdeviceBackendUnifiedTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "DummyBackend.h"
#include "LogicalNameMappingBackend.h"

#include <boost/thread/barrier.hpp>

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(DoubleBufferingBackendTestSuite)

/**********************************************************************************************************************/

// We need a test that is not run as unit test and which is used as long-running data consistency check
// - perhalps call CGs python scripts as init for hw?
//   firmware provides double buffering region with continuously changing data (counter signal)
struct DeviceFixture_HW {
  Device d;
  ScalarRegisterAccessor<uint32_t> doubleBufferingEnabled, writingBufferNum, fifoStatus;
  std::map<int16_t, unsigned> jumpHist; // for a historgram  jump -> count in corrupt data

  DeviceFixture_HW() {
    setDMapFilePath("doubleBufferHW.dmap");
    d.open("logicalDev");
    doubleBufferingEnabled.replace(d.getScalarRegisterAccessor<uint32_t>("enableDoubleBuffering"));
    writingBufferNum.replace(d.getScalarRegisterAccessor<uint32_t>("currentBufferNumber"));
    fifoStatus.replace(d.getScalarRegisterAccessor<uint32_t>("fifoStatus"));
  }

  // try several times (with random sleeps inbetween) to read signal from reg and return
  // the number of times signal was detected as corrupted
  unsigned checkDataCorruption(std::string reg, unsigned tries) {
    unsigned short goodStep = 100;
    bool writeCorruptData = true;
    unsigned dataCorruptionCount = 0;
    auto accessorA = d.getOneDRegisterAccessor<int16_t>(reg);

    for(unsigned t = 0; t < tries; t++) {
      accessorA.readLatest();
      fifoStatus.readLatest();

      unsigned i = 0;
      int16_t previousVal = 0;
      for(int16_t val : accessorA) {
        // val is allowed to increase by goodStep or wrap-around
        if(previousVal != 0 && val != (int16_t)(previousVal + goodStep)) {
          std::cout << "found data corruption at index " << i << ": step from " << previousVal << " to " << val
                    << " while DAQ fifoStatus=" << fifoStatus << std::endl;
          if(writeCorruptData) {
            char fname[100];
            snprintf(fname, sizeof(fname), "corruptData%03i.dat", i);
            std::cout << "writing corrupt data to " << fname << std::endl;
            std::ofstream f(fname);
            for(int16_t val1 : accessorA) {
              f << val1 << std::endl;
            }
            f.close();
          }
          dataCorruptionCount++;
          jumpHist[(int16_t)(val - previousVal)]++;
        }
        previousVal = val;
        i++;
      }
      // random sleep of [0..0.1] seconds
      boost::this_thread::sleep_for(boost::chrono::milliseconds(std::rand() % 100));
    };
    return dataCorruptionCount;
  }
  void enableDoubleBuf(bool ena) {
    doubleBufferingEnabled = ena;
    doubleBufferingEnabled.write();
  }
  unsigned getActiveBufferNo() {
    writingBufferNum.readLatest();
    return writingBufferNum;
  }
  void printHist() {
    std::cout << "histogram of wrong count-up values in data:" << std::endl;
    for(auto p : jumpHist) {
      std::cout << " distance=" << p.first << " :  " << p.second << " times" << std::endl;
    }
  }
};

BOOST_FIXTURE_TEST_CASE(testWithHardware0, DeviceFixture_HW) {
  /*
   * Here we check that we can actually detect corrupted data, if double buffer feature is off
   * This is a prerequesite for following test to make sense.
   * Note, we would even expect corrup data when double buffering still on, if we read to currently written
   * buffer, only less often.
   */
  enableDoubleBuf(false);
  unsigned bufferNo = getActiveBufferNo();
  // read from the buffer which is currently written to
  unsigned dataCorruptionCount = checkDataCorruption(bufferNo == 0 ? "channel10buf0" : "channel10buf1", 200);
  printHist();
  BOOST_CHECK(dataCorruptionCount > 0);
}

BOOST_FIXTURE_TEST_CASE(testWithHardware1, DeviceFixture_HW) {
  /*
   * Here we look for data corruption when firmware uses double buffering.
   * This test is long-running.
   *
   * Note, this test will probably fail even with double-buffering enabled.
   * This happens if data loss appears due to congestion in dma controller. This must be solved on the firmware side.
   * For discussion see https://redmine.msktools.desy.de/issues/10522
   */
  unsigned dataCorruptionCount = checkDataCorruption("channel10", 1000);
  printHist();
  BOOST_CHECK(dataCorruptionCount == 0);
}

/********************************************************************************************
*************************/

BOOST_AUTO_TEST_SUITE_END()
