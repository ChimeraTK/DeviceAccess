// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

/**********************************************************************************************************************/
/* Keep this file in a way that the tests also run with real hardware.                                                */
/**********************************************************************************************************************/

#include "BackendFactory.h"
#include "boost_dynamic_init_test.h"
#include "Device.h"
#include "DMapFileParser.h"
#include "NumericAddress.h"
#include "RebotBackend.h"
#include "Utilities.h"

namespace ChimeraTK {
  using namespace ChimeraTK;
}
using namespace boost::unit_test_framework;
using ChimeraTK::numeric_address::BAR;

typedef ChimeraTK::DeviceInfoMap::DeviceInfo DeviceInfo;

/**********************************************************************************************************************/
struct RebotServerDetails {
  std::string ip;
  std::string port;

  RebotServerDetails() = default;
  RebotServerDetails(std::string& ipAddress, std::string portNumber) : ip(ipAddress), port(portNumber) {}
};

class RebotTestClass {
 private:
  std::string _cardAlias;
  RebotServerDetails _rebotServer;

 public:
  explicit RebotTestClass(std::string const& cardAlias);
  void testConnection();
  void testReadWriteAPIOfRebotBackend();

 private:
  /*
   * parse the relevant dmap file to extract ip and port which would be required
   * for testing the rebot backend
   */
  RebotServerDetails getServerDetails(const std::string& cardAlias);

  DeviceInfo getDeviceDetailsFromDMap(const std::string& cardAlias);
  RebotServerDetails extractServerDetailsFromUri(std::string& uri);

  void checkWriteReadFromRegister(ChimeraTK::Device& rebotDevice);
};

/**********************************************************************************************************************/
class RebotDeviceTestSuite : public test_suite {
 public:
  explicit RebotDeviceTestSuite(std::string const& cardAlias) : test_suite("RebotDeviceTestSuite") {
    boost::shared_ptr<RebotTestClass> rebotTest(new RebotTestClass(cardAlias));
    add(BOOST_CLASS_TEST_CASE(&RebotTestClass::testConnection, rebotTest));
    add(BOOST_CLASS_TEST_CASE(&RebotTestClass::testReadWriteAPIOfRebotBackend, rebotTest));
  }
};

bool init_unit_test() {
  if(framework::master_test_suite().argc < 2) {
    std::cout << "Usage: " << framework::master_test_suite().argv[0] << " cardAlias [dmapFile]" << std::endl;
    return false;
  }
  auto cardAlias = framework::master_test_suite().argv[1];

  // take dmap file location if given, else search for cardAlias in the
  // factory default dmap file
  if(framework::master_test_suite().argc > 2) { // there is a second argument
    ChimeraTK::BackendFactory::getInstance().setDMapFilePath(framework::master_test_suite().argv[2]);
  }

  framework::master_test_suite().p_name.value = "Rebot backend test suite";
  framework::master_test_suite().add(new RebotDeviceTestSuite(cardAlias));
  return true;
}

/**********************************************************************************************************************/

RebotTestClass::RebotTestClass(std::string const& cardAlias)
: _cardAlias(cardAlias), _rebotServer(getServerDetails(cardAlias)) {}

RebotServerDetails RebotTestClass::getServerDetails(const std::string& cardAlias) {
  DeviceInfo deviceDetails = getDeviceDetailsFromDMap(cardAlias);
  return extractServerDetailsFromUri(deviceDetails.uri);
}

DeviceInfo RebotTestClass::getDeviceDetailsFromDMap(const std::string& cardAlias) {
  std::string dmapFileLocation = ChimeraTK::BackendFactory::getInstance().getDMapFilePath();
  ChimeraTK::DMapFileParser dMapParser;
  boost::shared_ptr<ChimeraTK::DeviceInfoMap> listOfDevicesInDMapFile;
  listOfDevicesInDMapFile = dMapParser.parse(dmapFileLocation);
  ChimeraTK::DeviceInfoMap::DeviceInfo deviceDetails;
  listOfDevicesInDMapFile->getDeviceInfo(cardAlias, deviceDetails);
  return deviceDetails;
}

RebotServerDetails RebotTestClass::extractServerDetailsFromUri(std::string& uri) {
  ChimeraTK::DeviceDescriptor parsedSDM = ChimeraTK::Utilities::parseDeviceDesciptor(uri);
  std::map<std::string, std::string>& serverParameters = parsedSDM.parameters;
  std::string ip = serverParameters["ip"];
  std::string port = serverParameters["port"];
  return RebotServerDetails(ip, port);
}

void RebotTestClass::testConnection() { // BAckend test

  // create connection with good ip and port see that there are no exceptions
  ChimeraTK::RebotBackend rebotBackend(_rebotServer.ip, _rebotServer.port, "", 30);
  BOOST_CHECK_EQUAL(rebotBackend.isOpen(), false);

  BOOST_CHECK_NO_THROW(rebotBackend.open());
  BOOST_CHECK_EQUAL(rebotBackend.isOpen(), true);

  // it must always be possible to call open() again
  BOOST_CHECK_NO_THROW(rebotBackend.open());
  BOOST_CHECK_EQUAL(rebotBackend.isOpen(), true);

  BOOST_CHECK_NO_THROW(rebotBackend.close());
  BOOST_CHECK_EQUAL(rebotBackend.isOpen(), false);

  // it must always be possible to call close() again
  BOOST_CHECK_NO_THROW(rebotBackend.close());
  BOOST_CHECK_EQUAL(rebotBackend.isOpen(), false);
}

void RebotTestClass::testReadWriteAPIOfRebotBackend() {
  ChimeraTK::RebotBackend rebotBackend(_rebotServer.ip, _rebotServer.port);
  rebotBackend.open();

  // The dummy server wites 0xDEADDEAD to the start address 0x04. Use this for testing.
  uint32_t address = 0x04;
  int32_t readValue = 0;
  rebotBackend.read(uint64_t(0), address, &readValue, sizeof(readValue));
  BOOST_CHECK_EQUAL(0xDEADBEEF, readValue);

  /********************************************************************************************************************/
  // Single word read -  Hardcoding addresses for now
  uint64_t word_status_register_address = 0x8;
  int32_t data = -987;
  // Register
  rebotBackend.write(0, word_status_register_address, &data, sizeof(data));

  rebotBackend.read(0, word_status_register_address, &readValue, sizeof(readValue));

  BOOST_CHECK_EQUAL(data, readValue);
  /********************************************************************************************************************/

  // Multiword read/write
  uint32_t word_clk_mux_addr = 28;
  int32_t dataToWrite[4] = {rand(), rand(), rand(), rand()};
  int32_t readInData[4];

  rebotBackend.write(0, word_clk_mux_addr, dataToWrite, sizeof(dataToWrite));
  rebotBackend.read(0, word_clk_mux_addr, readInData, sizeof(readInData));

  for(int i = 0; i < 4; i++) {
    BOOST_CHECK_EQUAL(dataToWrite[i], readInData[i]);
  }

  uint32_t test_area_Addr = 0x00000030;
  std::vector<int32_t> test_area_data(1024);
  for(uint32_t i = 0; i < test_area_data.size(); ++i) {
    test_area_data.at(i) = i;
  }
  std::vector<int32_t> test_area_ReadIndata(1024);

  rebotBackend.write(0, test_area_Addr, test_area_data.data(), sizeof(int32_t) * test_area_data.size());
  rebotBackend.read(0, test_area_Addr, test_area_ReadIndata.data(), sizeof(int32_t) * test_area_ReadIndata.size());

  for(uint32_t i = 0; i < test_area_ReadIndata.size(); i++) {
    BOOST_CHECK_EQUAL(test_area_data[i], test_area_ReadIndata[i]);
  }
}
