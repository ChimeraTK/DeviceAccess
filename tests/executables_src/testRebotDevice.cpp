#include <boost/test/included/unit_test.hpp>
#include "BackendFactory.h"
using namespace boost::unit_test_framework;

typedef mtca4u::DeviceInfoMap::DeviceInfo DeviceInfo;

class RebotTestClass;
class RebotDeviceTestSuite;

struct RebotServerDetails{
  std::string ip;
  int port;
};

/******************************************************************************/
boost::unit_test::test_suite* init_unit_test_suite(int argc, char** argv) {
  if (argc < 2) {

    std::cout << "Usage: " << argv[0] << " cardAlias [dmapFile]" << std::endl;

  } else if (bool hasSecondArgument = (argv[2] != nullptr)) {

    // take dmap file location if given, else search for cardAlias in the
    // factory default dmap file
    mtca4u::BackendFactory::getInstance().setDMapFilePath(argv[2]);

  }
  return new RebotDeviceTestSuite(argv[1]);

}
/******************************************************************************/

class RebotDeviceTestSuite : public test_suite {
public:
  RebotDeviceTestSuite(std::string const& cardAlias)
          : test_suite("RebotDeviceTestSuite") {
    boost::shared_ptr<RebotTestClass> rebotTest(new RebotTestClass(cardAlias));
  }
};

/******************************************************************************/

class RebotTestClass {
private:
  std::string _cardAlias;
  RebotServerDetails _rebotServer;

public:
  RebotTestClass(std::string const& cardAlias);

private:
  /*
   * parse the relevant dmap file to extract ip and port which would be required
   * for testing the rebot backend
   */
  RebotServerDetails getServerDetails();
  DeviceInfo getDeviceDetailsFromDMap();
  RebotServerDetails extractServerDetailsFromUri(std::string &uri);
};

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

inline RebotServerDetails RebotTestClass::extractServerDetailsFromUri(
    std::string& uri) {

}
