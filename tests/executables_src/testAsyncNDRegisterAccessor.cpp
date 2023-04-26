// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE AsyncSmokeTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "DummyBackend.h"
using namespace ChimeraTK;

#include <future>

// Note: Most functionality of the AsyncNDRegisterAccessor is tested in testNumericAddressedBackendUnified

BOOST_AUTO_TEST_CASE(Test_setWriteAccessor) {
  Device d("(dummy?map=goodMapFile.map)");
  d.open();
  d.activateAsyncRead();

  auto asyncAccessor = d.getScalarRegisterAccessor<int>("MODULE0/INTERRUPT_TYPE", 0, {AccessMode::wait_for_new_data});

  auto syncAccessor = d.getScalarRegisterAccessor<int>("MODULE0/INTERRUPT_TYPE/DUMMY_WRITEABLE", 0);
  auto syncAccessor_ptr = boost::dynamic_pointer_cast<NDRegisterAccessor<int>>(syncAccessor.get()->shared_from_this());
  auto asyncAccessor_casted = dynamic_cast<AsyncNDRegisterAccessor<int>*>(asyncAccessor.get());

  asyncAccessor_casted->setWriteAccessor(syncAccessor_ptr);

  BOOST_CHECK(!asyncAccessor.isReadOnly());
  BOOST_CHECK(asyncAccessor.isReadable());
  BOOST_CHECK(asyncAccessor.isWriteable());

  asyncAccessor.read(); // the initial value has arrived
  auto isReadFinished = std::async(std::launch::async, [&] { asyncAccessor.read(); });

  BOOST_CHECK(isReadFinished.wait_for(std::chrono::seconds(1)) == std::future_status::timeout);
  auto dummy = boost::dynamic_pointer_cast<DummyBackend>(d.getBackend());
  dummy->triggerInterrupt(6); // the interrupt with data
  BOOST_CHECK(isReadFinished.wait_for(std::chrono::seconds(3)) == std::future_status::ready);

  BOOST_CHECK(int(asyncAccessor) != 43);

  asyncAccessor = 43;
  asyncAccessor.write();

  BOOST_CHECK_EQUAL(d.read<int>("MODULE0/INTERRUPT_TYPE"), 43);
}
