#include <boost/test/included/unit_test.hpp>
#include "BackendFactory.h"
#include "RebotBackend.h"

#include "Utilities.h"
#include "DMapFileParser.h"

using namespace boost::unit_test_framework;

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

  // the actual tests
  // Backend tests:
    // connection test
    // Multiple write
    // Multiple read
    // unaligned read/write
    // read/write when device closed
  void testConnection();



private:
  /*
   * parse the relevant dmap file to extract ip and port which would be required
   * for testing the rebot backend
   */
  RebotServerDetails getServerDetails();
  DeviceInfo getDeviceDetailsFromDMap();
  RebotServerDetails extractServerDetailsFromUri(std::string &uri);
};

/******************************************************************************/
class RebotDeviceTestSuite : public test_suite {
public:
  RebotDeviceTestSuite(std::string const& cardAlias)
          : test_suite("RebotDeviceTestSuite") {
    boost::shared_ptr<RebotTestClass> rebotTest(new RebotTestClass(cardAlias));
    add(BOOST_CLASS_TEST_CASE(&RebotTestClass::testConnection, rebotTest));
  }
};

boost::unit_test::test_suite* init_unit_test_suite(int argc, char** argv) {
  if (argc < 2) {

    std::cout << "Usage: " << argv[0] << " cardAlias [dmapFile]" << std::endl;

  }
  bool hasSecondArgument = (argv[2] != nullptr);
  if (hasSecondArgument) {

    // take dmap file location if given, else search for cardAlias in the
    // factory default dmap file
    mtca4u::BackendFactory::getInstance().setDMapFilePath(argv[2]);

  }
  return new RebotDeviceTestSuite(argv[1]);

}

/******************************************************************************/


RebotTestClass::RebotTestClass(std::string const& cardAlias)
    : _cardAlias(cardAlias), _rebotServer(getServerDetails()) {}

RebotServerDetails RebotTestClass::getServerDetails() {
  DeviceInfo deviceDetails = getDeviceDetailsFromDMap();
  return extractServerDetailsFromUri(deviceDetails.uri);
}

DeviceInfo RebotTestClass::getDeviceDetailsFromDMap() {
  std::string dmapFileLocation =
      mtca4u::BackendFactory::getInstance().getDMapFilePath();
  mtca4u::DMapFileParser dMapParser;
  boost::shared_ptr<mtca4u::DeviceInfoMap> listOfDevicesInDMapFile;
  listOfDevicesInDMapFile = dMapParser.parse(dmapFileLocation);
  mtca4u::DeviceInfoMap::DeviceInfo deviceDetails;
  // FIXME: change the signature below? return instead of return through reference
  listOfDevicesInDMapFile->getDeviceInfo(_cardAlias, deviceDetails);
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

void RebotTestClass::testConnection() {

  // create connection with good ip and port see that there are no exceptions
  mtca4u::RebotBackend goodConnection(_rebotServer.ip, _rebotServer.port);
  mtca4u::RebotBackend goodConnection1(_rebotServer.ip, _rebotServer.port);
  BOOST_CHECK_EQUAL(goodConnection.isConnected(), true);
  BOOST_CHECK_EQUAL(goodConnection.isOpen(), false);
  BOOST_CHECK_NO_THROW(goodConnection.open());
  goodConnection1.open();
  BOOST_CHECK_NO_THROW(goodConnection.close());
}
