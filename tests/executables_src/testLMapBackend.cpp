// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LMapBackendTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "DummyRegisterAccessor.h"
#include "ExceptionDummyBackend.h"
#include "TransferGroup.h"
#include "UnifiedBackendTest.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(LMapBackendTestSuite)

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testExceptions) {
  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  ChimeraTK::Device device;
  BOOST_CHECK(device.isOpened() == false);
  device.open("LMAP0");
  BOOST_CHECK(device.isOpened() == true);
  // you must always be able to re-open a backend. It should try to re-connect, if applicable.
  device.open();
  BOOST_CHECK(device.isOpened() == true);
  device.open("LMAP0");
  BOOST_CHECK(device.isOpened() == true);

  int data = 0;

  BOOST_CHECK_THROW(device.write("Channel3", data), ChimeraTK::logic_error);

  BOOST_CHECK_THROW(device.getOneDRegisterAccessor<int>("ExceedsNumberOfChannels"), ChimeraTK::logic_error);
  BOOST_CHECK_NO_THROW(device.getOneDRegisterAccessor<int>("LastChannelInRegister"));

  BOOST_CHECK(device.isOpened() == true);
  device.close();
  BOOST_CHECK(device.isOpened() == false);
  device.close();
  BOOST_CHECK(device.isOpened() == false);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testCatalogue) {
  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  ChimeraTK::Device device;

  device.open("LMAP0");

  const RegisterCatalogue& catalogue = device.getRegisterCatalogue();

  auto info = catalogue.getRegister("SingleWord");
  BOOST_CHECK(info.getRegisterName() == "/SingleWord");
  BOOST_CHECK(info.getNumberOfElements() == 1);
  BOOST_CHECK(info.getNumberOfChannels() == 1);
  BOOST_CHECK(info.getNumberOfDimensions() == 0);

  info = catalogue.getRegister("FullArea");
  BOOST_CHECK(info.getRegisterName() == "/FullArea");
  BOOST_CHECK(info.getNumberOfElements() == 0x400);
  BOOST_CHECK(info.getNumberOfChannels() == 1);
  BOOST_CHECK(info.getNumberOfDimensions() == 1);

  info = catalogue.getRegister("PartOfArea");
  BOOST_CHECK(info.getRegisterName() == "/PartOfArea");
  BOOST_CHECK(info.getNumberOfElements() == 20);
  BOOST_CHECK(info.getNumberOfChannels() == 1);
  BOOST_CHECK(info.getNumberOfDimensions() == 1);

  ChimeraTK::Device target1;
  target1.open("PCIE3");
  ChimeraTK::TwoDRegisterAccessor<int32_t> accTarget = target1.getTwoDRegisterAccessor<int32_t>("TEST/NODMA");
  unsigned int nSamples = accTarget[3].size();

  info = catalogue.getRegister("Channel3");
  BOOST_CHECK(info.getRegisterName() == "/Channel3");
  BOOST_CHECK(info.getNumberOfElements() == nSamples);
  BOOST_CHECK(info.getNumberOfChannels() == 1);
  BOOST_CHECK(info.getNumberOfDimensions() == 1);

  info = catalogue.getRegister("Constant2");
  BOOST_CHECK(info.getRegisterName() == "/Constant2");
  BOOST_CHECK(info.getNumberOfElements() == 1);
  BOOST_CHECK(info.getNumberOfChannels() == 1);
  BOOST_CHECK(info.getNumberOfDimensions() == 0);

  info = catalogue.getRegister("/MyModule/SomeSubmodule/Variable");
  BOOST_CHECK(info.getRegisterName() == "/MyModule/SomeSubmodule/Variable");
  BOOST_CHECK(info.getNumberOfElements() == 1);
  BOOST_CHECK(info.getNumberOfChannels() == 1);
  BOOST_CHECK(info.getNumberOfDimensions() == 0);

  // std::unordered_set<std::string> targetDevices = lmap.getTargetDevices();
  // BOOST_CHECK(targetDevices.size() == 2);
  // BOOST_CHECK(targetDevices.count("PCIE2") == 1);
  // BOOST_CHECK(targetDevices.count("PCIE3") == 1);

  device.close();
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadWriteConstant) {
  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  ChimeraTK::Device device;

  device.open("LMAP0");
  BOOST_CHECK(device.read<int>("Constant") == 42);

  BOOST_CHECK_THROW(device.write("Constant", 0), ChimeraTK::logic_error);

  BOOST_CHECK(device.read<int>("Constant") == 42);

  // test with buffering register accessor
  auto acc = device.getOneDRegisterAccessor<int32_t>("Constant");
  BOOST_CHECK(acc.getNElements() == 1);
  BOOST_CHECK(acc[0] == 0); // values are only available after the first read, otherwise there is the value after
                            // construction (= int32_t() aka. 0)
  acc.read();
  BOOST_CHECK(acc[0] == 42);
  BOOST_CHECK_THROW(acc.write(), ChimeraTK::logic_error);

  auto acc2 = device.getOneDRegisterAccessor<int32_t>("Constant");
  auto acc3 = device.getOneDRegisterAccessor<int32_t>("Constant2");

  boost::shared_ptr<NDRegisterAccessor<int32_t>> impl, impl2, impl3;
  impl = boost::dynamic_pointer_cast<NDRegisterAccessor<int32_t>>(acc.getHighLevelImplElement());
  impl2 = boost::dynamic_pointer_cast<NDRegisterAccessor<int32_t>>(acc2.getHighLevelImplElement());
  impl3 = boost::dynamic_pointer_cast<NDRegisterAccessor<int32_t>>(acc3.getHighLevelImplElement());

  // BOOST_CHECK( impl->mayReplaceOther(impl2) == true );    // this is
  // currently always set to false, since it doesn't really make any
  // difference...
  BOOST_CHECK(impl->mayReplaceOther(impl3) == false);

  auto arrayConstant = device.getOneDRegisterAccessor<int>("/ArrayConstant");
  BOOST_CHECK_EQUAL(arrayConstant.getNElements(), 5);
  BOOST_CHECK_EQUAL(arrayConstant[0], 0);
  BOOST_CHECK_EQUAL(arrayConstant[1], 0);
  BOOST_CHECK_EQUAL(arrayConstant[2], 0);
  BOOST_CHECK_EQUAL(arrayConstant[3], 0);
  BOOST_CHECK_EQUAL(arrayConstant[4], 0);
  arrayConstant.read();
  BOOST_CHECK_EQUAL(arrayConstant[0], 1111);
  BOOST_CHECK_EQUAL(arrayConstant[1], 2222);
  BOOST_CHECK_EQUAL(arrayConstant[2], 3333);
  BOOST_CHECK_EQUAL(arrayConstant[3], 4444);
  BOOST_CHECK_EQUAL(arrayConstant[4], 5555);
  BOOST_CHECK_THROW(arrayConstant.write(), ChimeraTK::logic_error);

  auto partOfArrayConstant = device.getOneDRegisterAccessor<int>("/ArrayConstant", 2, 1);
  BOOST_CHECK_EQUAL(partOfArrayConstant.getNElements(), 2);
  BOOST_CHECK_EQUAL(partOfArrayConstant[0], 0);
  BOOST_CHECK_EQUAL(partOfArrayConstant[1], 0);
  partOfArrayConstant.read();
  BOOST_CHECK_EQUAL(partOfArrayConstant[0], 2222);
  BOOST_CHECK_EQUAL(partOfArrayConstant[1], 3333);
  BOOST_CHECK_THROW(partOfArrayConstant.write(), ChimeraTK::logic_error);

  device.close();
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadWriteVariable) {
  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  ChimeraTK::Device device;

  device.open("LMAP0");

  // test with buffering register accessor
  auto acc = device.getOneDRegisterAccessor<int32_t>("/MyModule/SomeSubmodule/Variable");
  auto acc2 = device.getOneDRegisterAccessor<int32_t>("/MyModule/SomeSubmodule/Variable");
  BOOST_CHECK(acc.getVersionNumber() == VersionNumber(nullptr));
  BOOST_CHECK(acc2.getVersionNumber() == VersionNumber(nullptr));
  BOOST_CHECK(acc.getNElements() == 1);
  BOOST_CHECK(acc[0] == 0);
  BOOST_CHECK(acc2[0] == 0);
  acc.read();
  BOOST_CHECK(acc[0] == 2);
  acc[0] = 3;
  BOOST_CHECK(acc[0] == 3);
  BOOST_CHECK(acc2[0] == 0);
  acc.write();
  acc2.read();
  BOOST_CHECK(acc[0] == 3);
  BOOST_CHECK(acc2[0] == 3);

  // test array access
  auto arrayVariable = device.getOneDRegisterAccessor<int>("/ArrayVariable");
  BOOST_CHECK_EQUAL(arrayVariable.getNElements(), 6);
  BOOST_CHECK_EQUAL(arrayVariable[0], 0);
  BOOST_CHECK_EQUAL(arrayVariable[1], 0);
  BOOST_CHECK_EQUAL(arrayVariable[2], 0);
  BOOST_CHECK_EQUAL(arrayVariable[3], 0);
  BOOST_CHECK_EQUAL(arrayVariable[4], 0);
  BOOST_CHECK_EQUAL(arrayVariable[5], 0);
  arrayVariable.read();
  BOOST_CHECK_EQUAL(arrayVariable[0], 11);
  BOOST_CHECK_EQUAL(arrayVariable[1], 22);
  BOOST_CHECK_EQUAL(arrayVariable[2], 33);
  BOOST_CHECK_EQUAL(arrayVariable[3], 44);
  BOOST_CHECK_EQUAL(arrayVariable[4], 55);
  BOOST_CHECK_EQUAL(arrayVariable[5], 66);
  arrayVariable = std::vector<int>({6, 5, 4, 3, 2, 1});
  arrayVariable.write();
  BOOST_CHECK_EQUAL(arrayVariable[0], 6);
  BOOST_CHECK_EQUAL(arrayVariable[1], 5);
  BOOST_CHECK_EQUAL(arrayVariable[2], 4);
  BOOST_CHECK_EQUAL(arrayVariable[3], 3);
  BOOST_CHECK_EQUAL(arrayVariable[4], 2);
  BOOST_CHECK_EQUAL(arrayVariable[5], 1);
  arrayVariable = std::vector<int>({0, 0, 0, 0, 0, 0});
  arrayVariable.read();
  BOOST_CHECK_EQUAL(arrayVariable[0], 6);
  BOOST_CHECK_EQUAL(arrayVariable[1], 5);
  BOOST_CHECK_EQUAL(arrayVariable[2], 4);
  BOOST_CHECK_EQUAL(arrayVariable[3], 3);
  BOOST_CHECK_EQUAL(arrayVariable[4], 2);
  BOOST_CHECK_EQUAL(arrayVariable[5], 1);

  auto partOfArrayVariable = device.getOneDRegisterAccessor<int>("/ArrayVariable", 3, 2);
  BOOST_CHECK_EQUAL(partOfArrayVariable.getNElements(), 3);
  BOOST_CHECK_EQUAL(partOfArrayVariable[0], 0);
  BOOST_CHECK_EQUAL(partOfArrayVariable[1], 0);
  BOOST_CHECK_EQUAL(partOfArrayVariable[2], 0);
  partOfArrayVariable.read();
  BOOST_CHECK_EQUAL(partOfArrayVariable[0], 4);
  BOOST_CHECK_EQUAL(partOfArrayVariable[1], 3);
  BOOST_CHECK_EQUAL(partOfArrayVariable[2], 2);
  partOfArrayVariable = std::vector<int>({42, 120, 31415});
  partOfArrayVariable.write();
  BOOST_CHECK_EQUAL(partOfArrayVariable[0], 42);
  BOOST_CHECK_EQUAL(partOfArrayVariable[1], 120);
  BOOST_CHECK_EQUAL(partOfArrayVariable[2], 31415);
  partOfArrayVariable = std::vector<int>({0, 0, 0});
  partOfArrayVariable.read();
  BOOST_CHECK_EQUAL(partOfArrayVariable[0], 42);
  BOOST_CHECK_EQUAL(partOfArrayVariable[1], 120);
  BOOST_CHECK_EQUAL(partOfArrayVariable[2], 31415);

  BOOST_CHECK_EQUAL(arrayVariable[0], 6);
  BOOST_CHECK_EQUAL(arrayVariable[1], 5);
  BOOST_CHECK_EQUAL(arrayVariable[2], 4);
  BOOST_CHECK_EQUAL(arrayVariable[3], 3);
  BOOST_CHECK_EQUAL(arrayVariable[4], 2);
  BOOST_CHECK_EQUAL(arrayVariable[5], 1);
  arrayVariable.read();
  BOOST_CHECK_EQUAL(arrayVariable[0], 6);
  BOOST_CHECK_EQUAL(arrayVariable[1], 5);
  BOOST_CHECK_EQUAL(arrayVariable[2], 42);
  BOOST_CHECK_EQUAL(arrayVariable[3], 120);
  BOOST_CHECK_EQUAL(arrayVariable[4], 31415);
  BOOST_CHECK_EQUAL(arrayVariable[5], 1);

  device.close();
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadWriteRegister) {
  std::vector<int> area(1024);

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  ChimeraTK::Device device, target1;

  target1.open("PCIE2");
  device.open("LMAP0");
  // single word
  target1.write("BOARD.WORD_USER", 120);
  BOOST_CHECK(device.read<int>("SingleWord") == 120);

  target1.write("BOARD.WORD_USER", 66);
  BOOST_CHECK(device.read<int>("SingleWord") == 66);

  device.write("SingleWord", 42);
  BOOST_CHECK(target1.read<int>("BOARD.WORD_USER") == 42);

  device.write("SingleWord", 12);
  BOOST_CHECK(target1.read<int>("BOARD.WORD_USER") == 12);

  // area
  for(int i = 0; i < 1024; i++) area[i] = 12345 + 3 * i;
  target1.write("ADC.AREA_DMAABLE", area);
  area = device.read<int>("FullArea", 1024);
  for(int i = 0; i < 1024; i++) BOOST_CHECK(area[i] == 12345 + 3 * i);

  for(int i = 0; i < 1024; i++) area[i] = -876543210 + 42 * i;
  target1.write("ADC.AREA_DMAABLE", area);
  area = device.read<int>("FullArea", 1024);
  for(int i = 0; i < 1024; i++) BOOST_CHECK(area[i] == -876543210 + 42 * i);

  for(int i = 0; i < 1024; i++) area[i] = 12345 + 3 * i;
  device.write("FullArea", area);
  area = target1.read<int>("ADC.AREA_DMAABLE", 1024);
  for(int i = 0; i < 1024; i++) BOOST_CHECK(area[i] == 12345 + 3 * i);

  for(int i = 0; i < 1024; i++) area[i] = -876543210 + 42 * i;
  device.write("FullArea", area);
  area = target1.read<int>("ADC.AREA_DMAABLE", 1024);
  for(int i = 0; i < 1024; i++) BOOST_CHECK(area[i] == -876543210 + 42 * i);

  device.close();
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadWriteRange) {
  std::vector<int> area(1024);

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  ChimeraTK::Device device, target1;

  device.open("LMAP0");
  target1.open("PCIE2");

  for(int i = 0; i < 1024; i++) area[i] = 0;
  for(int i = 0; i < 20; i++) area[i + 10] = 12345 + 3 * i;
  target1.write("ADC.AREA_DMAABLE", area);
  area = device.read<int>("PartOfArea", 20);
  for(int i = 0; i < 20; i++) BOOST_CHECK(area[i] == 12345 + 3 * i);

  area.resize(1024);
  for(int i = 0; i < 1024; i++) area[i] = 0;
  for(int i = 0; i < 20; i++) area[i + 10] = -876543210 + 42 * i;
  target1.write("ADC.AREA_DMAABLE", area);
  for(int i = 0; i < 1024; i++) area[i] = 0;
  area = device.read<int>("PartOfArea", 20);
  for(int i = 0; i < 20; i++) BOOST_CHECK(area[i] == -876543210 + 42 * i);

  device.close();
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRegisterAccessorForRegister) {
  std::vector<int> area(1024);
  int index;

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  ChimeraTK::Device device, target1;

  device.open("LMAP0");
  target1.open("PCIE2");

  auto acc = device.getOneDRegisterAccessor<int32_t>("FullArea");
  BOOST_CHECK(!acc.isReadOnly());
  BOOST_CHECK(acc.isReadable());
  BOOST_CHECK(acc.isWriteable());

  auto acc2 = device.getOneDRegisterAccessor<int32_t>("PartOfArea");

  boost::shared_ptr<NDRegisterAccessor<int32_t>> impl, impl2;
  impl = boost::dynamic_pointer_cast<NDRegisterAccessor<int32_t>>(acc.getHighLevelImplElement());
  impl2 = boost::dynamic_pointer_cast<NDRegisterAccessor<int32_t>>(acc2.getHighLevelImplElement());

  BOOST_CHECK(impl != impl2);
  BOOST_CHECK(impl->mayReplaceOther(impl) == true);
  BOOST_CHECK(impl2->mayReplaceOther(impl) == false);
  BOOST_CHECK(impl->mayReplaceOther(impl2) == false);

  const ChimeraTK::OneDRegisterAccessor<int32_t> acc_const = acc;

  // reading via [] operator
  for(int i = 0; i < 1024; i++) area[i] = 12345 + 3 * i;
  target1.write("ADC.AREA_DMAABLE", area);
  acc.read();
  for(int i = 0; i < 1024; i++) BOOST_CHECK(acc[i] == 12345 + 3 * i);

  for(int i = 0; i < 1024; i++) area[i] = -876543210 + 42 * i;
  target1.write("ADC.AREA_DMAABLE", area);
  acc.read();
  for(int i = 0; i < 1024; i++) BOOST_CHECK(acc[i] == -876543210 + 42 * i);

  // writing via [] operator
  for(int i = 0; i < 1024; i++) acc[i] = 12345 + 3 * i;
  acc.write();
  area = target1.read<int>("ADC.AREA_DMAABLE", 1024);
  for(int i = 0; i < 1024; i++) BOOST_CHECK(area[i] == 12345 + 3 * i);

  for(int i = 0; i < 1024; i++) acc[i] = -876543210 + 42 * i;
  acc.write();
  area = target1.read<int>("ADC.AREA_DMAABLE", 1024);
  for(int i = 0; i < 1024; i++) BOOST_CHECK(area[i] == -876543210 + 42 * i);

  // reading via iterator
  index = 0;
  for(auto it = acc.begin(); it != acc.end(); ++it) {
    BOOST_CHECK(*it == -876543210 + 42 * index);
    ++index;
  }
  BOOST_CHECK(index == 1024);

  // reading via const_iterator
  index = 0;
  for(auto it = acc_const.begin(); it != acc_const.end(); ++it) {
    BOOST_CHECK(*it == -876543210 + 42 * index);
    ++index;
  }
  BOOST_CHECK(index == 1024);

  // reading via reverse_iterator
  index = 1024;
  for(auto it = acc.rbegin(); it != acc.rend(); ++it) {
    --index;
    BOOST_CHECK(*it == -876543210 + 42 * index);
  }
  BOOST_CHECK(index == 0);

  // reading via const_reverse_iterator
  index = 1024;
  for(auto it = acc_const.rbegin(); it != acc_const.rend(); ++it) {
    --index;
    BOOST_CHECK(*it == -876543210 + 42 * index);
  }
  BOOST_CHECK(index == 0);

  // swap with std::vector
  std::vector<int32_t> vec(1024);
  acc.swap(vec);
  for(unsigned int i = 0; i < vec.size(); i++) {
    BOOST_CHECK(vec[i] == -876543210 + 42 * static_cast<signed>(i));
  }

  device.close();
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRegisterAccessorForRange) {
  std::vector<int> area(1024);
  int index;

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  ChimeraTK::Device device, target1;

  device.open("LMAP0");
  target1.open("PCIE2");

  auto acc = device.getOneDRegisterAccessor<int32_t>("PartOfArea");
  BOOST_CHECK(acc.isReadOnly() == false);
  BOOST_CHECK(acc.isReadable());
  BOOST_CHECK(acc.isWriteable());

  const ChimeraTK::OneDRegisterAccessor<int32_t> acc_const = acc;

  for(int i = 0; i < 20; i++) area[i + 10] = 12345 + 3 * i;
  target1.write("ADC.AREA_DMAABLE", area);
  acc.read();
  for(int i = 0; i < 20; i++) BOOST_CHECK(acc[i] == 12345 + 3 * i);

  for(int i = 0; i < 20; i++) area[i + 10] = -876543210 + 42 * i;
  target1.write("ADC.AREA_DMAABLE", area);
  acc.read();
  for(int i = 0; i < 20; i++) BOOST_CHECK(acc[i] == -876543210 + 42 * i);

  // reading via iterator
  index = 0;
  for(auto it = acc.begin(); it != acc.end(); ++it) {
    BOOST_CHECK(*it == -876543210 + 42 * index);
    ++index;
  }
  BOOST_CHECK(index == 20);

  // reading via const_iterator
  index = 0;
  for(auto it = acc_const.begin(); it != acc_const.end(); ++it) {
    BOOST_CHECK(*it == -876543210 + 42 * index);
    ++index;
  }
  BOOST_CHECK(index == 20);

  // reading via reverse_iterator
  index = 20;
  for(auto it = acc.rbegin(); it != acc.rend(); ++it) {
    --index;
    BOOST_CHECK(*it == -876543210 + 42 * index);
  }
  BOOST_CHECK(index == 0);

  // reading via const_reverse_iterator
  index = 20;
  for(auto it = acc_const.rbegin(); it != acc_const.rend(); ++it) {
    --index;
    BOOST_CHECK(*it == -876543210 + 42 * index);
  }
  BOOST_CHECK(index == 0);

  // writing
  for(int i = 0; i < 20; i++) acc[i] = 24507 + 33 * i;
  acc.write();
  area = target1.read<int>("ADC.AREA_DMAABLE", 1024);
  for(int i = 0; i < 20; i++) BOOST_CHECK(area[i + 10] == 24507 + 33 * i);

  device.close();
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRegisterAccessorForChannel) {
  std::vector<int> area(1024);

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  ChimeraTK::Device device, target1;

  device.open("LMAP0");
  target1.open("PCIE3");

  auto acc3 = device.getOneDRegisterAccessor<int32_t>("Channel3");
  auto acc4 = device.getOneDRegisterAccessor<int32_t>("Channel4");

  auto acc3_2 = device.getOneDRegisterAccessor<int32_t>("Channel3");

  boost::shared_ptr<NDRegisterAccessor<int32_t>> impl3, impl4, impl3_2;
  impl3 = boost::dynamic_pointer_cast<NDRegisterAccessor<int32_t>>(acc3.getHighLevelImplElement());
  impl4 = boost::dynamic_pointer_cast<NDRegisterAccessor<int32_t>>(acc4.getHighLevelImplElement());
  impl3_2 = boost::dynamic_pointer_cast<NDRegisterAccessor<int32_t>>(acc3_2.getHighLevelImplElement());
  BOOST_CHECK(impl3->mayReplaceOther(impl3_2) == true);
  BOOST_CHECK(impl3->mayReplaceOther(impl4) == false);

  auto accTarget = target1.getTwoDRegisterAccessor<int32_t>("TEST/NODMA");
  unsigned int nSamples = accTarget[3].size();
  BOOST_CHECK(accTarget[4].size() == nSamples);
  BOOST_CHECK(acc3.getNElements() == nSamples);
  BOOST_CHECK(acc4.getNElements() == nSamples);

  // fill target register
  for(unsigned int i = 0; i < nSamples; i++) {
    accTarget[3][i] = 3000 + i;
    accTarget[4][i] = 4000 - i;
  }
  accTarget.write();

  // clear channel accessor buffers
  for(unsigned int i = 0; i < nSamples; i++) {
    acc3[i] = 0;
    acc4[i] = 0;
  }

  // read channel accessors
  acc3.read();
  for(unsigned int i = 0; i < nSamples; i++) {
    BOOST_CHECK(acc3[i] == (signed)(3000 + i));
    BOOST_CHECK(acc4[i] == 0);
  }
  acc4.read();
  for(unsigned int i = 0; i < nSamples; i++) {
    BOOST_CHECK(acc3[i] == (signed)(3000 + i));
    BOOST_CHECK(acc4[i] == (signed)(4000 - i));
  }

  // read via iterators
  unsigned int idx = 0;
  for(auto it = acc3.begin(); it != acc3.end(); ++it) {
    BOOST_CHECK(*it == (signed)(3000 + idx));
    ++idx;
  }
  BOOST_CHECK(idx == nSamples);

  // read via const iterators
  const ChimeraTK::OneDRegisterAccessor<int32_t>& acc3_const = acc3;
  idx = 0;
  for(auto it = acc3_const.begin(); it != acc3_const.end(); ++it) {
    BOOST_CHECK(*it == (signed)(3000 + idx));
    ++idx;
  }
  BOOST_CHECK(idx == nSamples);

  // read via reverse iterators
  idx = nSamples;
  for(auto it = acc3.rbegin(); it != acc3.rend(); ++it) {
    --idx;
    BOOST_CHECK(*it == (signed)(3000 + idx));
  }
  BOOST_CHECK(idx == 0);

  // read via reverse const iterators
  idx = nSamples;
  for(auto it = acc3_const.rbegin(); it != acc3_const.rend(); ++it) {
    --idx;
    BOOST_CHECK(*it == (signed)(3000 + idx));
  }
  BOOST_CHECK(idx == 0);

  // swap into other vector
  std::vector<int> someVector(nSamples);
  acc3.swap(someVector);
  for(unsigned int i = 0; i < nSamples; i++) {
    BOOST_CHECK(someVector[i] == (signed)(3000 + i));
  }

  // write channel registers fails
  BOOST_CHECK(acc3.isReadOnly());
  BOOST_CHECK(acc3.isReadable());
  BOOST_CHECK(acc3.isWriteable() == false);

  BOOST_CHECK(acc4.isReadOnly());
  BOOST_CHECK(acc4.isReadable());
  BOOST_CHECK(acc4.isWriteable() == false);

  BOOST_CHECK_THROW(acc3.write(), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(acc4.write(), ChimeraTK::logic_error);

  device.close();
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRegisterAccessorForBit) {
  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  ChimeraTK::Device device;

  device.open("LMAP0");

  auto bitField = device.getScalarRegisterAccessor<int>("/MyModule/SomeSubmodule/Variable");
  auto bit0 = device.getScalarRegisterAccessor<uint8_t>("/Bit0ofVar");
  auto bit1 = device.getScalarRegisterAccessor<uint16_t>("/Bit1ofVar");
  auto bit2 = device.getScalarRegisterAccessor<int32_t>("/Bit2ofVar");
  auto bit3 = device.getScalarRegisterAccessor<std::string>("/Bit3ofVar");

  bitField = 0;
  bitField.write();

  bit0.read();
  BOOST_CHECK_EQUAL(static_cast<uint8_t>(bit0), 0);
  bit1.read();
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(bit1), 0);
  bit2.read();
  BOOST_CHECK_EQUAL(static_cast<int32_t>(bit2), 0);
  bit3.read();
  BOOST_CHECK_EQUAL(static_cast<std::string>(bit3), "0");

  bitField = 1;
  bitField.write();

  bit0.read();
  BOOST_CHECK_EQUAL(static_cast<uint8_t>(bit0), 1);
  bit1.read();
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(bit1), 0);
  bit2.read();
  BOOST_CHECK_EQUAL(static_cast<int32_t>(bit2), 0);
  bit3.read();
  BOOST_CHECK_EQUAL(static_cast<std::string>(bit3), "0");

  bitField = 2;
  bitField.write();

  bit0.read();
  BOOST_CHECK_EQUAL(static_cast<uint8_t>(bit0), 0);
  bit1.read();
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(bit1), 1);
  bit2.read();
  BOOST_CHECK_EQUAL(static_cast<int32_t>(bit2), 0);
  bit3.read();
  BOOST_CHECK_EQUAL(static_cast<std::string>(bit3), "0");

  bitField = 3;
  bitField.write();

  bit0.read();
  BOOST_CHECK_EQUAL(static_cast<uint8_t>(bit0), 1);
  bit1.read();
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(bit1), 1);
  bit2.read();
  BOOST_CHECK_EQUAL(static_cast<int32_t>(bit2), 0);
  bit3.read();
  BOOST_CHECK_EQUAL(static_cast<std::string>(bit3), "0");

  bitField = 4;
  bitField.write();

  bit0.read();
  BOOST_CHECK_EQUAL(static_cast<uint8_t>(bit0), 0);
  bit1.read();
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(bit1), 0);
  bit2.read();
  BOOST_CHECK_EQUAL(static_cast<int32_t>(bit2), 1);
  bit3.read();
  BOOST_CHECK_EQUAL(static_cast<std::string>(bit3), "0");

  bitField = 8;
  bitField.write();

  bit0.read();
  BOOST_CHECK_EQUAL(static_cast<uint8_t>(bit0), 0);
  bit1.read();
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(bit1), 0);
  bit2.read();
  BOOST_CHECK_EQUAL(static_cast<int32_t>(bit2), 0);
  bit3.read();
  BOOST_CHECK_EQUAL(static_cast<std::string>(bit3), "1");

  bitField = 15;
  bitField.write();

  bit0.read();
  BOOST_CHECK_EQUAL(static_cast<uint8_t>(bit0), 1);
  bit1.read();
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(bit1), 1);
  bit2.read();
  BOOST_CHECK_EQUAL(static_cast<int32_t>(bit2), 1);
  bit3.read();
  BOOST_CHECK_EQUAL(static_cast<std::string>(bit3), "1");

  bitField = 16;
  bitField.write();

  bit0.read();
  BOOST_CHECK_EQUAL(static_cast<uint8_t>(bit0), 0);
  bit1.read();
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(bit1), 0);
  bit2.read();
  BOOST_CHECK_EQUAL(static_cast<int32_t>(bit2), 0);
  bit3.read();
  BOOST_CHECK_EQUAL(static_cast<std::string>(bit3), "0");

  bitField = 17;
  bitField.write();

  bit0.read();
  BOOST_CHECK_EQUAL(static_cast<uint8_t>(bit0), 1);
  bit1.read();
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(bit1), 0);
  bit2.read();
  BOOST_CHECK_EQUAL(static_cast<int32_t>(bit2), 0);
  bit3.read();
  BOOST_CHECK_EQUAL(static_cast<std::string>(bit3), "0");

  bitField = 1;
  bitField.write();

  bit0.read();
  BOOST_CHECK_EQUAL(static_cast<uint8_t>(bit0), 1);
  bit1.read();
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(bit1), 0);
  bit2.read();
  BOOST_CHECK_EQUAL(static_cast<int32_t>(bit2), 0);
  bit3.read();
  BOOST_CHECK_EQUAL(static_cast<std::string>(bit3), "0");

  bit2 = 1;
  bit2.write();
  bitField.read();
  BOOST_CHECK_EQUAL(static_cast<int>(bitField), 5);

  bit1 = 1;
  bit1.write();
  bitField.read();
  BOOST_CHECK_EQUAL(static_cast<int>(bitField), 7);

  bit0 = 0;
  bit0.write();
  bitField.read();
  BOOST_CHECK_EQUAL(static_cast<int>(bitField), 6);

  bit3 = "1";
  bit3.write();
  bitField.read();
  BOOST_CHECK_EQUAL(static_cast<int>(bitField), 14);

  // Test with TransferGroup
  TransferGroup group;
  group.addAccessor(bit0);
  group.addAccessor(bit1);
  group.addAccessor(bit2);
  group.addAccessor(bit3);

  bitField = 0;
  bitField.write();

  group.read();
  BOOST_CHECK_EQUAL(static_cast<uint8_t>(bit0), 0);
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(bit1), 0);
  BOOST_CHECK_EQUAL(static_cast<int32_t>(bit2), 0);
  BOOST_CHECK_EQUAL(static_cast<std::string>(bit3), "0");

  bitField = 1;
  bitField.write();

  group.read();
  BOOST_CHECK_EQUAL(static_cast<uint8_t>(bit0), 1);
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(bit1), 0);
  BOOST_CHECK_EQUAL(static_cast<int32_t>(bit2), 0);
  BOOST_CHECK_EQUAL(static_cast<std::string>(bit3), "0");

  bitField = 2;
  bitField.write();

  group.read();
  BOOST_CHECK_EQUAL(static_cast<uint8_t>(bit0), 0);
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(bit1), 1);
  BOOST_CHECK_EQUAL(static_cast<int32_t>(bit2), 0);
  BOOST_CHECK_EQUAL(static_cast<std::string>(bit3), "0");

  bitField = 3;
  bitField.write();

  group.read();
  BOOST_CHECK_EQUAL(static_cast<uint8_t>(bit0), 1);
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(bit1), 1);
  BOOST_CHECK_EQUAL(static_cast<int32_t>(bit2), 0);
  BOOST_CHECK_EQUAL(static_cast<std::string>(bit3), "0");

  bitField = 4;
  bitField.write();

  group.read();
  BOOST_CHECK_EQUAL(static_cast<uint8_t>(bit0), 0);
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(bit1), 0);
  BOOST_CHECK_EQUAL(static_cast<int32_t>(bit2), 1);
  BOOST_CHECK_EQUAL(static_cast<std::string>(bit3), "0");

  bitField = 8;
  bitField.write();

  group.read();
  BOOST_CHECK_EQUAL(static_cast<uint8_t>(bit0), 0);
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(bit1), 0);
  BOOST_CHECK_EQUAL(static_cast<int32_t>(bit2), 0);
  BOOST_CHECK_EQUAL(static_cast<std::string>(bit3), "1");

  bitField = 15;
  bitField.write();

  group.read();
  BOOST_CHECK_EQUAL(static_cast<uint8_t>(bit0), 1);
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(bit1), 1);
  BOOST_CHECK_EQUAL(static_cast<int32_t>(bit2), 1);
  BOOST_CHECK_EQUAL(static_cast<std::string>(bit3), "1");

  bitField = 16;
  bitField.write();

  group.read();
  BOOST_CHECK_EQUAL(static_cast<uint8_t>(bit0), 0);
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(bit1), 0);
  BOOST_CHECK_EQUAL(static_cast<int32_t>(bit2), 0);
  BOOST_CHECK_EQUAL(static_cast<std::string>(bit3), "0");

  bitField = 17;
  bitField.write();

  group.read();
  BOOST_CHECK_EQUAL(static_cast<uint8_t>(bit0), 1);
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(bit1), 0);
  BOOST_CHECK_EQUAL(static_cast<int32_t>(bit2), 0);
  BOOST_CHECK_EQUAL(static_cast<std::string>(bit3), "0");

  bitField = 1;
  bitField.write();

  group.read();
  BOOST_CHECK_EQUAL(static_cast<uint8_t>(bit0), 1);
  BOOST_CHECK_EQUAL(static_cast<uint16_t>(bit1), 0);
  BOOST_CHECK_EQUAL(static_cast<int32_t>(bit2), 0);
  BOOST_CHECK_EQUAL(static_cast<std::string>(bit3), "0");

  bit2 = 1;
  group.write();
  bitField.read();
  BOOST_CHECK_EQUAL(static_cast<int>(bitField), 5);

  bit1 = 1;
  group.write();
  bitField.read();
  BOOST_CHECK_EQUAL(static_cast<int>(bitField), 7);

  bit0 = 0;
  group.write();
  bitField.read();
  BOOST_CHECK_EQUAL(static_cast<int>(bitField), 6);

  bit3 = "1";
  group.write();
  bitField.read();
  BOOST_CHECK_EQUAL(static_cast<int>(bitField), 14);

  device.close();
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testOther) {
  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  ChimeraTK::Device device;
  device.open("LMAP0");

  BOOST_CHECK(device.readDeviceInfo().find("Logical name mapping file:") == 0);
  device.close();
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testParameters) {
  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  ChimeraTK::Device device;

  device.open("PARAMS0");

  BOOST_CHECK_EQUAL(device.read<int>("SingleWordWithParams"), 42);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testAccessorPlugins) {
  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  ChimeraTK::Device device, target;

  device.open("LMAP0");
  target.open("PCIE2");

  // test scalar register with multiply plugin
  auto wordUser = target.getScalarRegisterAccessor<int32_t>("BOARD.WORD_USER");
  auto wordUserScaled = device.getScalarRegisterAccessor<double>("SingleWord_Scaled");

  wordUser = 2;
  wordUser.write();
  wordUserScaled.read();
  BOOST_CHECK_CLOSE(double(wordUserScaled), 2 * 4.2, 0.001);

  wordUser = 3;
  wordUser.write();
  wordUserScaled.read();
  BOOST_CHECK_CLOSE(double(wordUserScaled), 3 * 4.2, 0.001);

  wordUserScaled = 10 / 4.2;
  wordUserScaled.write();
  wordUser.read();
  BOOST_CHECK_EQUAL(int(wordUser), 10);

  wordUserScaled = 5.4 / 4.2; // rounding down
  wordUserScaled.write();
  wordUser.read();
  BOOST_CHECK_EQUAL(int(wordUser), 5);

  wordUserScaled = 3.6 / 4.2; // rounding up
  wordUserScaled.write();
  wordUser.read();
  BOOST_CHECK_EQUAL(int(wordUser), 4);

  wordUserScaled = -5.4 / 4.2; // rounding down
  wordUserScaled.write();
  wordUser.read();
  BOOST_CHECK_EQUAL(int(wordUser), -5);

  wordUserScaled = -3.6 / 4.2; // rounding up
  wordUserScaled.write();
  wordUser.read();
  BOOST_CHECK_EQUAL(int(wordUser), -4);

  // test scalar register with two multiply plugins
  auto wordUserScaledTwice = device.getScalarRegisterAccessor<double>("SingleWord_Scaled_Twice");

  wordUser = 2;
  wordUser.write();
  wordUserScaledTwice.read();
  BOOST_CHECK_CLOSE(double(wordUserScaledTwice), 2 * 6, 0.001);

  wordUser = 3;
  wordUser.write();
  wordUserScaledTwice.read();
  BOOST_CHECK_CLOSE(double(wordUserScaledTwice), 3 * 6, 0.001);

  wordUserScaledTwice = 10. / 6.;
  wordUserScaledTwice.write();
  wordUser.read();
  BOOST_CHECK_EQUAL(int(wordUser), 10);

  // test array register with multiply plugin
  auto area = target.getOneDRegisterAccessor<int32_t>("ADC.AREA_DMAABLE");
  auto areaScaled = device.getOneDRegisterAccessor<double>("FullArea_Scaled");

  BOOST_CHECK_EQUAL(area.getNElements(), 1024);
  BOOST_CHECK_EQUAL(areaScaled.getNElements(), 1024);

  for(int i = 0; i < 1024; ++i) area[i] = 100 + i;
  area.write();
  areaScaled.read();
  for(int i = 0; i < 1024; ++i) BOOST_CHECK_CLOSE(areaScaled[i], (100 + i) * 0.5, 0.001);

  for(int i = 0; i < 1024; ++i) areaScaled[i] = (-100 + i) / 0.5;
  areaScaled.write();
  area.read();
  for(int i = 0; i < 1024; ++i) BOOST_CHECK_EQUAL(area[i], -100 + i);
}

/********************************************************************************************************************/
BOOST_AUTO_TEST_CASE(testIsFunctional) {
  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  auto exceptionDummyBackend = boost::dynamic_pointer_cast<ExceptionDummy>(
      BackendFactory::getInstance().createBackend("(ExceptionDummy:1?map=test3.map)"));

  Device d{"LMAP1"};
  d.open();
  BOOST_CHECK(d.isFunctional() == true);

  d.setException("Test Exception");
  BOOST_CHECK(d.isFunctional() == false);

  d.open();
  BOOST_CHECK(d.isFunctional() == true);

  d.close();
  BOOST_CHECK(d.isFunctional() == false);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testWithTransferGroup) {
  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  ChimeraTK::Device device, target1, target2;

  device.open("LMAP0");
  OneDRegisterAccessor<int> a[6];
  a[0].replace(device.getOneDRegisterAccessor<int>("SingleWord"));
  a[1].replace(device.getOneDRegisterAccessor<int>("FullArea"));
  a[2].replace(device.getOneDRegisterAccessor<int>("PartOfArea"));
  a[3].replace(device.getOneDRegisterAccessor<int>("Channel3"));
  a[4].replace(device.getOneDRegisterAccessor<int>("Channel4"));
  a[5].replace(device.getOneDRegisterAccessor<int>("Constant"));

  // obtain the private pointers to the implementation of the accessor
  boost::shared_ptr<NDRegisterAccessor<int>> impl[6];
  for(int i = 0; i < 6; i++) {
    impl[i] = boost::dynamic_pointer_cast<NDRegisterAccessor<int>>(a[i].getHighLevelImplElement());
  }

  // somewhat redundant check: underlying hardware accessors are different for
  // all accessors
  for(int i = 0; i < 6; i++) {
    BOOST_CHECK(impl[i]->getHardwareAccessingElements().size() == 1);
    for(int k = i + 1; k < 6; k++) {
      BOOST_CHECK(impl[i]->getHardwareAccessingElements()[0] != impl[k]->getHardwareAccessingElements()[0]);
    }
  }

  // add accessors to the transfer group
  TransferGroup group;
  for(int i = 0; i < 6; i++) {
    group.addAccessor(a[i]);
  }

  // now some accessors share the same underlying accessor
  BOOST_CHECK(impl[3]->getHardwareAccessingElements()[0] == impl[4]->getHardwareAccessingElements()[0]);
  BOOST_CHECK(impl[1]->getHardwareAccessingElements()[0] == impl[2]->getHardwareAccessingElements()[0]);

  // the others are still different
  BOOST_CHECK(impl[0]->getHardwareAccessingElements()[0] != impl[1]->getHardwareAccessingElements()[0]);
  BOOST_CHECK(impl[0]->getHardwareAccessingElements()[0] != impl[3]->getHardwareAccessingElements()[0]);
  BOOST_CHECK(impl[0]->getHardwareAccessingElements()[0] != impl[5]->getHardwareAccessingElements()[0]);
  BOOST_CHECK(impl[1]->getHardwareAccessingElements()[0] != impl[3]->getHardwareAccessingElements()[0]);
  BOOST_CHECK(impl[1]->getHardwareAccessingElements()[0] != impl[5]->getHardwareAccessingElements()[0]);
  BOOST_CHECK(impl[3]->getHardwareAccessingElements()[0] != impl[5]->getHardwareAccessingElements()[0]);

  // write some stuff to the registers via the target device
  // Note: there is only one DMA area in the PCIE dummy which are shared by the
  // registers accessed by t2 and t3. We
  //       therefore cannot test those register at the same time!
  target1.open("PCIE2");
  target2.open("PCIE3");
  auto t1 = target1.getOneDRegisterAccessor<int>("BOARD.WORD_USER");
  auto t2 = target1.getOneDRegisterAccessor<int>("ADC.AREA_DMAABLE");
  auto t3 = target2.getTwoDRegisterAccessor<int>("TEST/NODMA");

  t1[0] = 120;
  t1.write();
  for(unsigned int i = 0; i < t2.getNElements(); i++) {
    t2[i] = 67890 + 66 * signed(i);
  }
  t2.write();

  // read it back via the transfer group
  group.read();

  BOOST_CHECK(a[0][0] == 120);

  BOOST_CHECK(t2.getNElements() == a[1].getNElements());
  for(unsigned int i = 0; i < t2.getNElements(); i++) {
    BOOST_CHECK(a[1][i] == 67890 + 66 * signed(i));
  }

  BOOST_CHECK(a[2].getNElements() == 20);
  for(unsigned int i = 0; i < a[2].getNElements(); i++) {
    BOOST_CHECK(a[2][i] == 67890 + 66 * signed(i + 10));
  }

  BOOST_CHECK(a[5][0] == 42);

  // write something to the multiplexed 2d register
  for(unsigned int i = 0; i < t3.getNChannels(); i++) {
    for(unsigned int k = 0; k < t3[i].size(); k++) {
      t3[i][k] = signed(i * 10 + k);
    }
  }
  t3.write();

  // read it back via transfer group
  group.read();

  BOOST_CHECK(a[3].getNElements() == t3[3].size());
  for(unsigned int i = 0; i < a[3].getNElements(); i++) {
    BOOST_CHECK(a[3][i] == 3 * 10 + signed(i));
  }

  BOOST_CHECK(a[4].getNElements() == t3[4].size());
  for(unsigned int i = 0; i < a[4].getNElements(); i++) {
    BOOST_CHECK(a[4][i] == 4 * 10 + signed(i));
  }

  // check that writing to the group fails (has read-only elements)
  BOOST_CHECK_THROW(group.write(), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
