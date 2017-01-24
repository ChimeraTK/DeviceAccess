/******************************************************************************
 * Keep this file in a way that the tests also run with real hardware.
 ******************************************************************************/

#include <boost/test/included/unit_test.hpp>
#include "BackendFactory.h"
#include "RebotBackend.h"
#include "Device.h"

#include "Utilities.h"
#include "DMapFileParser.h"
#include "NumericAddress.h"

using namespace boost::unit_test_framework;
using mtca4u::numeric_address::BAR;

typedef mtca4u::DeviceInfoMap::DeviceInfo DeviceInfo;

/******************************************************************************/
struct RebotServerDetails{
  std::string ip;
  int port;

  RebotServerDetails() : ip(), port(0) {};
  RebotServerDetails(std::string& ipAddress, int portNumber)
      : ip(ipAddress), port(portNumber) {};
};

class RebotTestClass {
private:
  std::string _cardAlias;
  RebotServerDetails _rebotServer;

public:
  RebotTestClass(std::string const& cardAlias);
  void testConnection();
  void testReadWriteAPIOfRebotBackend();

private:
  /*
  * parse the relevant dmap file to extract ip and port which would be required
  * for testing the rebot backend
  */
  RebotServerDetails getServerDetails(const std::string& cardAlias);

  DeviceInfo getDeviceDetailsFromDMap(const std::string& cardAlias);
  RebotServerDetails extractServerDetailsFromUri(std::string &uri);

  void checkWriteReadFromRegister(mtca4u::Device &rebotDevice);
};

/******************************************************************************/
class RebotDeviceTestSuite : public test_suite {
public:
  RebotDeviceTestSuite(std::string const& cardAlias)
          : test_suite("RebotDeviceTestSuite") {
    boost::shared_ptr<RebotTestClass> rebotTest(new RebotTestClass(cardAlias));
    add(BOOST_CLASS_TEST_CASE(&RebotTestClass::testConnection, rebotTest));
    add(BOOST_CLASS_TEST_CASE(&RebotTestClass::testReadWriteAPIOfRebotBackend, rebotTest));
  }
};

boost::unit_test::test_suite* init_unit_test_suite(int argc, char** argv) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " cardAlias [dmapFile]" << std::endl;
    return nullptr;
  }
  bool hasSecondArgument = (argv[2] != nullptr);

  // take dmap file location if given, else search for cardAlias in the
  // factory default dmap file
  if (hasSecondArgument) {
    mtca4u::BackendFactory::getInstance().setDMapFilePath(argv[2]);
  }

  return new RebotDeviceTestSuite(argv[1]);

}

/******************************************************************************/

RebotTestClass::RebotTestClass(std::string const& cardAlias)
    : _cardAlias(cardAlias), _rebotServer(getServerDetails(cardAlias)) {}


RebotServerDetails RebotTestClass::getServerDetails(
    const std::string& cardAlias) {
  DeviceInfo deviceDetails = getDeviceDetailsFromDMap(cardAlias);
  return extractServerDetailsFromUri(deviceDetails.uri);
}

DeviceInfo RebotTestClass::getDeviceDetailsFromDMap(
    const std::string& cardAlias) {
  std::string dmapFileLocation =
      mtca4u::BackendFactory::getInstance().getDMapFilePath();
  mtca4u::DMapFileParser dMapParser;
  boost::shared_ptr<mtca4u::DeviceInfoMap> listOfDevicesInDMapFile;
  listOfDevicesInDMapFile = dMapParser.parse(dmapFileLocation);
  mtca4u::DeviceInfoMap::DeviceInfo deviceDetails;
  listOfDevicesInDMapFile->getDeviceInfo(cardAlias, deviceDetails);
  return deviceDetails;
}

RebotServerDetails RebotTestClass::extractServerDetailsFromUri(std::string& uri) {
  mtca4u::Sdm parsedSDM = mtca4u::Utilities::parseSdm(uri);
  std::list<std::string> &serverParameters = parsedSDM._Parameters;
  std::list<std::string>::iterator it = serverParameters.begin();
  std::string ip = *it;
  int port = std::stoi(*(++it));
  return RebotServerDetails(ip, port);
}

void RebotTestClass::testConnection() { // BAckend test

  // create connection with good ip and port see that there are no exceptions
  mtca4u::RebotBackend rebotBackend(_rebotServer.ip, _rebotServer.port);
  mtca4u::RebotBackend secondConnectionToServer(_rebotServer.ip, _rebotServer.port);
  BOOST_CHECK_EQUAL(rebotBackend.isConnected(), true);
  BOOST_CHECK_EQUAL(rebotBackend.isOpen(), false);

  BOOST_CHECK_NO_THROW(rebotBackend.open());
  BOOST_CHECK_EQUAL(rebotBackend.isConnected(), true);
  BOOST_CHECK_EQUAL(rebotBackend.isOpen(), true);

  //BOOST_CHECK_THROW(secondConnectionToServer.open(), mtca4u::RebotBackendException);

  BOOST_CHECK_NO_THROW(rebotBackend.close());
  BOOST_CHECK_EQUAL(rebotBackend.isConnected(), true);
  BOOST_CHECK_EQUAL(rebotBackend.isOpen(), false);
}

void RebotTestClass::testReadWriteAPIOfRebotBackend() {
  mtca4u::RebotBackend rebotBackend(_rebotServer.ip, _rebotServer.port);
  rebotBackend.open();

  /*****************
   * The dummy server wites 0xDEADDEAD to the start address 0x04. Use this for
   * testing
   */
  uint32_t address = 0x04;
  int32_t readValue = 0;
  rebotBackend.read(0, address, &readValue, sizeof(readValue));
  BOOST_CHECK_EQUAL(0xDEADBEEF, readValue);

  /****************************************************************************/
  // Single word read -  Hardcoding addresses for now
  uint32_t word_status_register_address = 0x8;
  int32_t data = -987;
  // Register
  rebotBackend.write(0, word_status_register_address, &data, sizeof(data));

  rebotBackend.read(0, word_status_register_address, &readValue, sizeof(readValue));

  BOOST_CHECK_EQUAL(data, readValue);
  /****************************************************************************/

  // Multiword read/write
  uint32_t word_clk_mux_addr = 28;
  int32_t dataToWrite[4] = {rand(), rand(), rand(), rand()};
  int32_t readInData[4];

  rebotBackend.write(0, word_clk_mux_addr, dataToWrite, sizeof(dataToWrite));
  rebotBackend.read(0, word_clk_mux_addr, readInData, sizeof(readInData));

  for(int i = 0; i < 4; i++){
    BOOST_CHECK_EQUAL(dataToWrite[i], readInData[i]);
  }

  uint32_t test_area_Addr = 0x00000030;
  std::vector<int32_t> test_area_data(1024);
  for (uint32_t i = 0; i < test_area_data.size(); ++i) {
    test_area_data.at(i) = i;
  }
  std::vector<int32_t> test_area_ReadIndata(1024);

  rebotBackend.write(0, test_area_Addr, test_area_data.data(), sizeof(int32_t) * test_area_data.size());
  rebotBackend.read(0, test_area_Addr, test_area_ReadIndata.data(), sizeof(int32_t) * test_area_ReadIndata.size());

  for(uint32_t i = 0; i < test_area_ReadIndata.size(); i++){
    BOOST_CHECK_EQUAL(test_area_data[i], test_area_ReadIndata[i]);
  }
}
