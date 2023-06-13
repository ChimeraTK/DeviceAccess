// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE OneDRegisterAccessorTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BackendFactory.h"
#include "Device.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"
#include "OneDRegisterAccessor.h"
#include "WriteCountingBackend.h"

#include <boost/bind/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/make_shared.hpp>

#include <algorithm>
#include <math.h>

using namespace boost::unit_test_framework;
using namespace ChimeraTK;

/**********************************************************************************************************************/

struct Fixture {
  Fixture() {
    setDMapFilePath("dummies.dmap");
    device.open("(WriteCountingDummy?map=goodMapFile.map)");
  }

  Device device;
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testRegisterAccessor, Fixture) {
  std::cout << "testRegisterAccessor" << std::endl;

  // obtain register accessor with integral type
  OneDRegisterAccessor<int> intRegister = device.getOneDRegisterAccessor<int>("APP0/MODULE0");
  BOOST_CHECK(intRegister.isReadOnly() == false);
  BOOST_CHECK(intRegister.isReadable());
  BOOST_CHECK(intRegister.isWriteable());

  // check number of elements getter
  BOOST_CHECK(intRegister.getNElements() == 3);

  // test operator[] on r.h.s.
  device.write<int>("APP0/MODULE0", std::vector<int>({5, -77, 99}));
  intRegister.read();
  BOOST_CHECK(intRegister[0] == 5);
  BOOST_CHECK(intRegister[1] == -77);
  BOOST_CHECK(intRegister[2] == 99);

  // test operator[] on l.h.s.
  intRegister[0] = -666;
  intRegister[1] = 999;
  intRegister[2] = 222;
  intRegister.write();
  BOOST_CHECK(device.read<int>("APP0/MODULE0", 3) == std::vector<int>({-666, 999, 222}));

  // test data() function
  int* ptr = intRegister.data();
  BOOST_CHECK(ptr[0] = -666);
  BOOST_CHECK(ptr[1] = 999);
  BOOST_CHECK(ptr[2] = 222);
  ptr[0] = 123;
  ptr[1] = 456;
  ptr[2] = 789;
  BOOST_CHECK(intRegister[0] = 123);
  BOOST_CHECK(intRegister[1] = 456);
  BOOST_CHECK(intRegister[2] = 789);

  // test iterators with begin and end
  int ic = 0;
  for(OneDRegisterAccessor<int>::iterator it = intRegister.begin(); it != intRegister.end(); ++it) {
    *it = 1000 * (ic + 1);
    ic++;
  }
  intRegister.write();
  BOOST_CHECK(device.read<int>("APP0/MODULE0", 3) == std::vector<int>({1000, 2000, 3000}));

  // test iterators with rbegin and rend
  ic = 0;
  for(OneDRegisterAccessor<int>::reverse_iterator it = intRegister.rbegin(); it != intRegister.rend(); ++it) {
    *it = 333 * (ic + 1);
    ic++;
  }
  intRegister.write();
  BOOST_CHECK(device.read<int>("APP0/MODULE0", 3) == std::vector<int>({999, 666, 333}));

  // test const iterators in both directions
  device.write("APP0/MODULE0", std::vector<int>({1234, 2468, 3702}));
  intRegister.read();
  const OneDRegisterAccessor<int> const_intRegister = intRegister;
  ic = 0;
  for(OneDRegisterAccessor<int>::const_iterator it = const_intRegister.begin(); it != const_intRegister.end(); ++it) {
    BOOST_CHECK(*it == 1234 * (ic + 1));
    ic++;
  }
  ic = 0;
  for(OneDRegisterAccessor<int>::const_reverse_iterator it = const_intRegister.rbegin(); it != const_intRegister.rend();
      ++it) {
    BOOST_CHECK(*it == 1234 * (3 - ic));
    ic++;
  }

  // test swap with std::vector
  std::vector<int> x(3);
  x[0] = 11;
  x[1] = 22;
  x[2] = 33;
  intRegister.swap(x);
  BOOST_CHECK(x[0] == 1234);
  BOOST_CHECK(x[1] == 2468);
  BOOST_CHECK(x[2] == 3702);
  BOOST_CHECK(intRegister[0] == 11);
  BOOST_CHECK(intRegister[1] == 22);
  BOOST_CHECK(intRegister[2] == 33);

  // obtain register accessor with fractional type, to check if fixed-point
  // conversion is working (3 fractional bits)
  OneDRegisterAccessor<double> floatRegister = device.getOneDRegisterAccessor<double>("MODULE0/WORD_USER1");

  // test operator[] on r.h.s.
  device.write("APP0/MODULE0", std::vector<int>({-120, 2468}));
  floatRegister.read();
  BOOST_CHECK(floatRegister[0] == -120. / 8.);

  // test operator[] on l.h.s.
  floatRegister[0] = 42. / 8.;
  floatRegister.write();
  BOOST_CHECK(device.read<int>("APP0/MODULE0", 2) == std::vector<int>({42, 2468}));
}

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testWriteIfDifferent, Fixture) {
  std::cout << "testWriteIfDifferent" << std::endl;

  OneDRegisterAccessor<int> accessor = device.getOneDRegisterAccessor<int>("APP0/MODULE0");

  // dummy register accessor for comparison
  auto backend = boost::dynamic_pointer_cast<WriteCountingBackend>(device.getBackend());
  BOOST_CHECK(backend != NULL);
  DummyRegisterAccessor<int> dummy(backend.get(), "APP0", "MODULE0");

  // Inital write and writeIfDifferent with same value
  accessor = {501, 502, 503};
  accessor.write();
  size_t counterBefore = backend->writeCount;
  accessor.writeIfDifferent({501, 502, 503}); // should not write
  size_t counterAfter = backend->writeCount;
  BOOST_CHECK(counterBefore == counterAfter);

  // writeIfDifferent with different value
  counterBefore = backend->writeCount;
  accessor.writeIfDifferent({501, 504, 503}); // should write
  counterAfter = backend->writeCount;
  BOOST_CHECK(counterAfter == counterBefore + 1);

  // writeIfDifferent with same value, but explicit version number
  counterBefore = backend->writeCount;
  accessor.writeIfDifferent({501, 504, 503}, VersionNumber{}); // should not write
  counterAfter = backend->writeCount;
  BOOST_CHECK(counterAfter == counterBefore);

  // writeIfDifferent with different value, and explicit version number
  counterBefore = backend->writeCount;
  accessor.writeIfDifferent({505, 504, 503}, VersionNumber{}); // should write
  counterAfter = backend->writeCount;
  BOOST_CHECK(counterAfter == counterBefore + 1);

  // writeIfDifferent with same value, but different DataValidity
  counterBefore = backend->writeCount;
  accessor.writeIfDifferent({505, 504, 503}, VersionNumber{nullptr}, DataValidity::faulty); // should write
  counterAfter = backend->writeCount;
  BOOST_CHECK(counterAfter == counterBefore + 1);

  // writeIfDifferent with same value, but different DataValidity (now back at OK)
  counterBefore = backend->writeCount;
  accessor.writeIfDifferent({505, 504, 503}); // should write
  counterAfter = backend->writeCount;
  BOOST_CHECK(counterAfter == counterBefore + 1);

  device.close();
}

/**********************************************************************************************************************/
