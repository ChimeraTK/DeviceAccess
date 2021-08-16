#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE AsyncSmokeTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "DummyBackend.h"
using namespace ChimeraTK;

#include <future>

// This is just a smoke test for basic functionality. Might quickly become obsolete with the UnifiedBackendTest
BOOST_AUTO_TEST_CASE(SmokeTest) {
  Device d("(dummy?map=goodMapFile.map)");
  d.open();
  d.activateAsyncRead();

  auto asyncAccessor =
      d.getScalarRegisterAccessor<double>("MODULE0/INTERRUPT_TYPE", 0, {AccessMode::wait_for_new_data});
  BOOST_CHECK(asyncAccessor.isReadOnly());
  BOOST_CHECK(asyncAccessor.isReadable());
  BOOST_CHECK(!asyncAccessor.isWriteable());

  auto asyncVoidAccessor = d.getVoidRegisterAccessor("MODULE0/INTERRUPT_VOID1", {AccessMode::wait_for_new_data});

  auto syncAccessor = d.getScalarRegisterAccessor<double>("MODULE0/INTERRUPT_TYPE", 0);
  BOOST_CHECK(syncAccessor.isReadOnly());
  BOOST_CHECK(syncAccessor.isReadable());
  BOOST_CHECK(!syncAccessor.isWriteable());

  auto writeableAccessor = d.getScalarRegisterAccessor<double>("MODULE0/INTERRUPT_TYPE/DUMMY_WRITEABLE", 0);
  BOOST_CHECK(writeableAccessor.isWriteable());
  writeableAccessor = 42;
  writeableAccessor.write();

  asyncAccessor.read();     // the initial value has arrived
  asyncVoidAccessor.read(); // does this get stuck?
  BOOST_CHECK_EQUAL(double(asyncAccessor), 0.0);

  auto isReadFinished = std::async(std::launch::async, [&] { asyncAccessor.read(); });
  auto isVoidReadFinished = std::async(std::launch::async, [&] { asyncVoidAccessor.read(); });

  BOOST_CHECK(isReadFinished.wait_for(std::chrono::seconds(1)) == std::future_status::timeout);
  BOOST_CHECK(isVoidReadFinished.wait_for(std::chrono::seconds(1)) == std::future_status::timeout);

  auto dummy = boost::dynamic_pointer_cast<DummyBackend>(d.getBackend());

  dummy->triggerInterrupt(5, 6); // the interrupt with data
  BOOST_CHECK(isReadFinished.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
  BOOST_CHECK_EQUAL(double(asyncAccessor), 42.0);
  BOOST_CHECK(isVoidReadFinished.wait_for(std::chrono::seconds(1)) == std::future_status::timeout);

  syncAccessor.read();
  BOOST_CHECK_EQUAL(double(syncAccessor), 42.0);

  // starrt another read of the async accessor. It will never see data
  isReadFinished = std::async(std::launch::async, [&] { asyncAccessor.read(); });

  dummy->triggerInterrupt(1, 3); // the void interrupt

  BOOST_CHECK(isVoidReadFinished.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
  BOOST_CHECK(isReadFinished.wait_for(std::chrono::seconds(1)) == std::future_status::timeout);

  // interrupt the blocking read of the accessor with data to get out of the program
  asyncAccessor.getHighLevelImplElement()->interrupt();
}
