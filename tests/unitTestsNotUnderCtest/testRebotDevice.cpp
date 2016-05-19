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
  void testWrite();
  void testFactory();

private:
  /*
  * parse the relevant dmap file to extract ip and port which would be required
  * for testing the rebot backend
  */
  RebotServerDetails getServerDetails();
  DeviceInfo getDeviceDetailsFromDMap();
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
    add(BOOST_CLASS_TEST_CASE(&RebotTestClass::testWrite, rebotTest));
    add(BOOST_CLASS_TEST_CASE(&RebotTestClass::testFactory, rebotTest));
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

void RebotTestClass::testWrite() {
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

inline void RebotTestClass::testFactory() {

  // There are four situations where the map-file information is coming from
  // 1. From the dmap file (old way, third column in dmap file)
  // 2. From the URI (new, recommended, not supported by dmap parser at the moment)
  // 3. No map file at all (not supported by the dmap parser at the moment)
  // 4. Both dmap file and URI contain the information (prints a warning and takes the one from the dmap file)

  // 1. The original way with map file as third column in the dmap file
  mtca4u::Device rebotDevice;
  rebotDevice.open(_cardAlias);
  checkWriteReadFromRegister(rebotDevice);
  rebotDevice.write<double>("BOARD/WORD_USER", 48 );
  rebotDevice.close(); // we have to close this because

  // 2. Creating without map file in the dmap only works by putting an sdm on creation because we have to bypass the
  // dmap file parser which at the time of writing this requires a map file as third column
  try{
    mtca4u::Device secondDevice;
    secondDevice.open("sdm://./rebot=localhost,5001,mtcadummy_rebot.map");
    BOOST_CHECK( secondDevice.read<double>("BOARD/WORD_USER")== 48 );
    secondDevice.close();
  }catch(mtca4u::DeviceException &e){
    BOOST_ERROR("Just an error, don't fail on exception during developnment");
  }

  // 3. We don't have a map file, so we have to use numerical addressing
  mtca4u::Device thirdDevice;
  thirdDevice.open("sdm://./rebot=localhost,5001");
  BOOST_CHECK( thirdDevice.read<int32_t>(BAR/0/0xC)== 48<<3 ); // The user register is on bar 0, address 0xC.
                                                               // We have no fixed point data conversion but 3 fractional bits.
  thirdDevice.close();

  // 4. This should print a warning. We can't check that, so we just check that it does work like the other two options.
  mtca4u::Device fourthDevice;
  fourthDevice.open("REBOT_DOUBLEMAP");
  BOOST_CHECK( fourthDevice.read<double>("BOARD/WORD_USER")== 48 );


}

void RebotTestClass::checkWriteReadFromRegister(
    mtca4u::Device& rebotDevice) {
  int32_t dataToWrite[4] = {2, 3, 100, 20};
  int32_t readInData[4];

  // 0xDEADBEEF is a word preset by the dummy firmware in the WORD_COMPILATION
  // register (addr 0x04). reading and verifying this register means the read
  // api of device acces works for the rebot device.
  rebotDevice.readReg("WORD_COMPILATION", "BOARD", readInData, sizeof(int32_t));
  BOOST_CHECK_EQUAL(0xDEADBEEF, readInData[0]);

  // ADC.WORLD_CLK_MUX is a 4 word/element register, this test would verify
  // write to the device through the api works. (THe read command has been
  // established to work by the read of the preset word).
  rebotDevice.writeReg("WORD_CLK_MUX", "ADC", dataToWrite, sizeof(dataToWrite));
  rebotDevice.readReg("WORD_CLK_MUX", "ADC", readInData, sizeof(readInData));
  for(int i = 0; i < 4; i++){
    BOOST_CHECK_EQUAL(dataToWrite[i], readInData[i]);
  }


  // test read from offset 2 on a multi word/element register.
  rebotDevice.readReg("WORD_CLK_MUX", "ADC", readInData, sizeof(int32_t), 2*sizeof(int32_t));
  BOOST_CHECK_EQUAL(dataToWrite[2], readInData[0]);

  // test write one element at offset position 2 on a multiword register.
  rebotDevice.writeReg("WORD_CLK_MUX", "ADC", dataToWrite, sizeof(int32_t), 2 * sizeof(int32_t));
  rebotDevice.readReg("WORD_CLK_MUX", "ADC", readInData, sizeof(int32_t), 2 * sizeof(int32_t));
  BOOST_CHECK_EQUAL(dataToWrite[0], readInData[0]);


  // test writing a continuous block from offset 1 in a multiword register.
  int32_t data[2] = {7896, 45678};
  rebotDevice.writeReg("WORD_CLK_MUX", "ADC", data, sizeof(data), 1 * sizeof(int32_t));
  rebotDevice.readReg("WORD_CLK_MUX", "ADC", readInData, sizeof(data), 1 * sizeof(int32_t));
  for(int i = 0; i < 2; i++){
    BOOST_CHECK_EQUAL(data[i], readInData[i]);
  }

  // test writing a continuous block from offset 1 in a multiword register
  // through an accessor
  data[0] = 676; data[1] = 9987; 
  auto accessor = rebotDevice.getRegisterAccessor("WORD_CLK_MUX", "ADC");
  accessor->write(data, 2, 1);
  accessor->read(readInData, 2, 1);
  for(int i = 0; i < 2; i++){
    BOOST_CHECK_EQUAL(data[i], readInData[i]);
  }


  // write to larger area using offsets in a loop
  auto testArea = rebotDevice.getRegisterAccessor(
      "TEST_AREA", "ADC"); // testArea is 1024 words long

  for (int i = 0; i < 10; ++i) {
    int value = i;
    testArea->write(&value, 1, i);
  }
  for (int i = 0; i < 10; ++i) {
    int value;
    testArea->read(&value, 1, i);
    BOOST_CHECK_EQUAL(value, i);
  }

}
