// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Device.h"
#include "Utilities.h"

#include <sys/time.h>

#include <fstream>
#include <iostream>

using namespace ChimeraTK;

/*
 *
 * Usage: ( cd tests ; ../bin/testAccessorPerformance [<NumberOfIterations>] )
 *
 * <NumberOfIterations> is the number of iterations used for block access tests.
 * Single word access tests will use 100000 times the given number of
 * iterations. If omitted, the number of iterations defaults to 10 (which is
 * acceptable also on slower machines in debug build mode).
 *
 */
int main(int argc, char** argv) {
  struct timeval tv;
  int64_t t0, tdur;

  setDMapFilePath("dummies.dmap");

  Device device;
  device.open("PERFTEST");

  int niterBlock;
  if(argc <= 1) {
    niterBlock = 10;
  }
  else {
    niterBlock = atoi(argv[1]);
  }

  int64_t sum = 0;

  std::ofstream fresult("performance_test.txt", std::ofstream::out);

  std::cout << " **************************************************************"
               "*************"
            << std::endl;
  std::cout << " Tests with the OneDRegisterAccessor:" << std::endl;

  auto acc1D = device.getOneDRegisterAccessor<int>("ADC/AREA_DMA_VIA_DMA");
  gettimeofday(&tv, nullptr);
  t0 = tv.tv_sec * 1000000 + tv.tv_usec;
  std::cout << " reading block ";
  for(int i = 0; i < niterBlock; ++i) {
    acc1D.read();
    sum += acc1D[i];
  }
  gettimeofday(&tv, nullptr);
  tdur = (tv.tv_sec * 1000000 + tv.tv_usec) - t0;
  std::cout << "took " << static_cast<double>(tdur) / 1000. / niterBlock << " ms per block" << std::endl;
  fresult << "1D_COOKEDus=" << std::round(static_cast<double>(tdur) / niterBlock) << std::endl;

  auto acc1Draw = device.getOneDRegisterAccessor<int>("ADC/AREA_DMA_VIA_DMA", 0, 0, {AccessMode::raw});
  gettimeofday(&tv, nullptr);
  t0 = tv.tv_sec * 1000000 + tv.tv_usec;
  std::cout << " raw-reading block ";
  for(int i = 0; i < niterBlock; ++i) {
    acc1Draw.read();
    sum += acc1Draw[i];
  }
  gettimeofday(&tv, nullptr);
  tdur = (tv.tv_sec * 1000000 + tv.tv_usec) - t0;
  std::cout << "took " << static_cast<double>(tdur) / 1000. / niterBlock << " ms per block" << std::endl;
  fresult << "1D_RAWus=" << std::round(static_cast<double>(tdur) / niterBlock) << std::endl;

  std::cout << " **************************************************************"
               "*************"
            << std::endl;
  std::cout << " Sum of all read data: " << sum << std::endl;

  fresult.close();

  return 0;
}
