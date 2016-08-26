
#define BOOST_TEST_MODULE SequenceDeMultiplexerTest
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <iostream>
#include <sstream>

#include "TwoDRegisterAccessor.h"
#include "NumericAddressedBackendMuxedRegisterAccessor.h"
#include "DummyBackend.h"
#include "MapFileParser.h"
#include "BackendFactory.h"
#include "MultiplexedDataAccessor.h"
#include "Device.h"

using namespace mtca4u;

static const std::string DMAP_FILE_NAME("dummies.dmap");
static const std::string DEVICE_ALIAS("SEQUENCES");
static const std::string DEVICE_INVALID_SEQUENCES_ALIAS("INVALID_SEQUENCES");
static const std::string DEVICE_MIXED_ALIAS("MIXED_SEQUENCES");

static const std::string MAP_FILE_NAME("sequences.map");
static const std::string TEST_MODULE_NAME("TEST");
static const std::string INVALID_MODULE_NAME("INVALID");
static const RegisterPath TEST_MODULE_PATH(TEST_MODULE_NAME);
static const RegisterPath INVALID_MODULE_PATH(INVALID_MODULE_NAME);

BOOST_AUTO_TEST_SUITE( SequenceDeMultiplexerTestSuite )

BOOST_AUTO_TEST_CASE( testConstructor ) {

    BackendFactory::getInstance().setDMapFilePath(DMAP_FILE_NAME);
    Device device;
    device.open(DEVICE_ALIAS);
    TwoDRegisterAccessor<double> deMultiplexer = device.getTwoDRegisterAccessor<double>(TEST_MODULE_PATH/"FRAC_INT");
    BOOST_CHECK( deMultiplexer[0].size() == 5 );

    device.close();
    device.open(DEVICE_INVALID_SEQUENCES_ALIAS);

    try {
      device.getTwoDRegisterAccessor<double>(INVALID_MODULE_PATH/"NO_WORDS");
      // in a sucessful test (which is required for the code coverage report)
      // the following line is not executed. Exclude it from the lcov report
      BOOST_ERROR( "getTwoDRegisterAccessor did not throw for NO_WORDS" ); //LCOV_EXCL_LINE
    }
    catch(MultiplexedDataAccessorException &e) {
      BOOST_CHECK( e.getID() == MultiplexedDataAccessorException::EMPTY_AREA );
    }

    try {
      device.getTwoDRegisterAccessor<double>(INVALID_MODULE_PATH/"WRONG_SIZE");
      BOOST_ERROR( "getTwoDRegisterAccessor did not throw for WRONG_SIZE" ); //LCOV_EXCL_LINE
    }
    catch(MultiplexedDataAccessorException &e) {
      BOOST_CHECK( e.getID() == MultiplexedDataAccessorException::INVALID_WORD_SIZE );
    }

    try {
      device.getTwoDRegisterAccessor<double>(INVALID_MODULE_PATH/"WRONG_NELEMENTS");
      BOOST_ERROR( "getTwoDRegisterAccessor did not throw for WRONG_NELEMENTS" ); //LCOV_EXCL_LINE
    }
    catch(MultiplexedDataAccessorException &e) {
      BOOST_CHECK( e.getID() == MultiplexedDataAccessorException::INVALID_N_ELEMENTS );
    }

    try {
      device.getTwoDRegisterAccessor<double>(INVALID_MODULE_PATH/"DOES_NOT_EXIST");
      BOOST_ERROR( "getTwoDRegisterAccessor did not throw for DOES_NOT_EXIST" ); //LCOV_EXCL_LINE
    }
    catch(DeviceException &e) {
      BOOST_CHECK( e.getID() == DeviceException::REGISTER_DOES_NOT_EXIST );
    }


}

// test the de-multiplexing itself, with 'identity' fixed point conversion
template <class SequenceWordType>
void testDeMultiplexing(std::string areaName) {

    // open a dummy device with the sequence map file
    BackendFactory::getInstance().setDMapFilePath(DMAP_FILE_NAME);
    Device device;
    device.open(DEVICE_ALIAS);

    //get the sequence info from the map file
    boost::shared_ptr<RegisterInfoMap> registerMap = MapFileParser().parse(MAP_FILE_NAME);
    SequenceInfo sequenceInfo;
    registerMap->getRegisterInfo("AREA_MULTIPLEXED_SEQUENCE_"+areaName, sequenceInfo, TEST_MODULE_NAME);

    std::vector< SequenceWordType > ioBuffer( sequenceInfo.nBytes/sizeof(SequenceWordType) );
    ioBuffer[0] = 'A';
    ioBuffer[1] = 'a';
    ioBuffer[2] = '0';
    ioBuffer[3] = 'B';
    ioBuffer[4] = 'b';
    ioBuffer[5] = '1';
    ioBuffer[6] = 'C';
    ioBuffer[7] = 'c';
    ioBuffer[8] = '2';
    ioBuffer[9] = 'D';
    ioBuffer[10] = 'd';
    ioBuffer[11] = '3';
    ioBuffer[12] = 'E';
    ioBuffer[13] = 'e';
    ioBuffer[14] = '4';

    device.writeArea(sequenceInfo.address, reinterpret_cast<int32_t*>( &(ioBuffer[0]) ), sequenceInfo.nBytes, sequenceInfo.bar);

    TwoDRegisterAccessor<SequenceWordType> deMultiplexer = device.getTwoDRegisterAccessor<SequenceWordType>(TEST_MODULE_PATH/areaName);

    BOOST_CHECK( deMultiplexer.isReadOnly() == false );
    BOOST_CHECK( deMultiplexer.isReadable() );
    BOOST_CHECK( deMultiplexer.isWriteable() );

    deMultiplexer.read();

    BOOST_CHECK( deMultiplexer[0][0] == 'A' );
    BOOST_CHECK( deMultiplexer[0][1] == 'B' );
    BOOST_CHECK( deMultiplexer[0][2] == 'C' );
    BOOST_CHECK( deMultiplexer[0][3] == 'D' );
    BOOST_CHECK( deMultiplexer[0][4] == 'E' );
    BOOST_CHECK( deMultiplexer[1][0] == 'a' );
    BOOST_CHECK( deMultiplexer[1][1] == 'b' );
    BOOST_CHECK( deMultiplexer[1][2] == 'c' );
    BOOST_CHECK( deMultiplexer[1][3] == 'd' );
    BOOST_CHECK( deMultiplexer[1][4] == 'e' );
    BOOST_CHECK( deMultiplexer[2][0] == '0' );
    BOOST_CHECK( deMultiplexer[2][1] == '1' );
    BOOST_CHECK( deMultiplexer[2][2] == '2' );
    BOOST_CHECK( deMultiplexer[2][3] == '3' );
    BOOST_CHECK( deMultiplexer[2][4] == '4' );

    for (size_t sequenceIndex=0; sequenceIndex<3; ++sequenceIndex){
      for(size_t i=0; i< 5; ++i){
        deMultiplexer[sequenceIndex][i]+=5;
      }
    }

    // currently the non-blocking read is not implemented in NumericAddressed accessors
    BOOST_CHECK_THROW( deMultiplexer.readNonBlocking(), DeviceException );

    deMultiplexer.write();
    device.readArea(sequenceInfo.address, reinterpret_cast<int32_t*>( &(ioBuffer[0]) ), sequenceInfo.nBytes, sequenceInfo.bar);

    BOOST_CHECK( ioBuffer[0] == 'F' );
    BOOST_CHECK( ioBuffer[1] == 'f' );
    BOOST_CHECK( ioBuffer[2] == '5' );
    BOOST_CHECK( ioBuffer[3] == 'G' );
    BOOST_CHECK( ioBuffer[4] == 'g' );
    BOOST_CHECK( ioBuffer[5] == '6' );
    BOOST_CHECK( ioBuffer[6] == 'H' );
    BOOST_CHECK( ioBuffer[7] == 'h' );
    BOOST_CHECK( ioBuffer[8] == '7' );
    BOOST_CHECK( ioBuffer[9] == 'I' );
    BOOST_CHECK( ioBuffer[10] == 'i' );
    BOOST_CHECK( ioBuffer[11] == '8' );
    BOOST_CHECK( ioBuffer[12] == 'J' );
    BOOST_CHECK( ioBuffer[13] == 'j' );
    BOOST_CHECK( ioBuffer[14] == '9' );

}

BOOST_AUTO_TEST_CASE(testDeMultiplexing32) {
  testDeMultiplexing<int32_t>("INT");
}
BOOST_AUTO_TEST_CASE(testDeMultiplexing16) {
  testDeMultiplexing<int16_t>("SHORT");
}
BOOST_AUTO_TEST_CASE(testDeMultiplexing8) {
  testDeMultiplexing<int8_t>("CHAR");
}

// test the de-multiplexing itself, with  fixed point conversion
// and using the factory function
template <class SequenceWordType>
void testWithConversion(std::string multiplexedSequenceName) {
    // open a dummy device with the sequence map file
    BackendFactory::getInstance().setDMapFilePath(DMAP_FILE_NAME);
    Device device;
    device.open(DEVICE_ALIAS);

    //get the sequence info from the map file
    boost::shared_ptr<RegisterInfoMap> registerMap = MapFileParser().parse(MAP_FILE_NAME);
    SequenceInfo sequenceInfo;
    registerMap->getRegisterInfo(MULTIPLEXED_SEQUENCE_PREFIX + multiplexedSequenceName, sequenceInfo, TEST_MODULE_NAME);

    std::vector< SequenceWordType > ioBuffer( sequenceInfo.nBytes/sizeof(SequenceWordType) );

    for (size_t i=0; i < ioBuffer.size(); ++i) {
      ioBuffer[i] = i;
    }

    device.writeArea(sequenceInfo.address, reinterpret_cast<int32_t*>( &(ioBuffer[0]) ), sequenceInfo.nBytes, sequenceInfo.bar);

    TwoDRegisterAccessor<float> accessor = device.getTwoDRegisterAccessor<float>(TEST_MODULE_PATH/multiplexedSequenceName);
    accessor.read();

    BOOST_CHECK( accessor[0][0] == 0 );
    BOOST_CHECK( accessor[1][0] == 0.25 ); // 1 with 2 frac bits
    BOOST_CHECK( accessor[2][0] == 0.25 ); // 2 with 3 frac bits
    BOOST_CHECK( accessor[0][1] == 1.5 ); // 3 with 1 frac bits
    BOOST_CHECK( accessor[1][1] == 1 ); // 4 with 2 frac bits
    BOOST_CHECK( accessor[2][1] == 0.625 ); // 5 with 3 frac bits
    BOOST_CHECK( accessor[0][2] == 3. ); // 6 with 1 frac bits
    BOOST_CHECK( accessor[1][2] == 1.75 ); // 7 with 2 frac bits
    BOOST_CHECK( accessor[2][2] == 1. ); // 8 with 3 frac bits
    BOOST_CHECK( accessor[0][3] == 4.5 ); // 9 with 1 frac bits
    BOOST_CHECK( accessor[1][3] == 2.5 ); // 10 with 2 frac bits
    BOOST_CHECK( accessor[2][3] == 1.375 ); // 11 with 3 frac bits
    BOOST_CHECK( accessor[0][4] == 6. ); // 12 with 1 frac bits
    BOOST_CHECK( accessor[1][4] == 3.25 ); // 13 with 2 frac bits
    BOOST_CHECK( accessor[2][4] == 1.75 ); // 14 with 3 frac bits

    for(size_t sequenceIndex=0; sequenceIndex<3; ++sequenceIndex) {
      for(size_t i=0; i< 5; ++i) {
        accessor[sequenceIndex][i]+=1.;
      }
    }

    accessor.write();
    device.readArea(sequenceInfo.address, reinterpret_cast<int32_t*>( &(ioBuffer[0]) ), sequenceInfo.nBytes, sequenceInfo.bar);

    for(size_t i=0; i < 15; ++i) {
      // with i%3+1 fractional bits the added floating point value of 1
      // corresponds to 2^(i%3+1) in fixed point represetation
      int addedValue = 1 << (i%3+1);
      std::stringstream message;
      message << "ioBuffer["<<i<< "] is " << ioBuffer[i] << ", expected " << i+addedValue;
      BOOST_CHECK_MESSAGE( ioBuffer[i] == static_cast< SequenceWordType>(i+addedValue) , message.str());
    }

    // currently the non-blocking read is not implemented in NumericAddressed accessors
    BOOST_CHECK_THROW( accessor.readNonBlocking(), DeviceException );
}

BOOST_AUTO_TEST_CASE(testWithConversion32) {
  testWithConversion<int32_t>("FRAC_INT");
}
BOOST_AUTO_TEST_CASE(testWithConversion16) {
  testWithConversion<int16_t>("FRAC_SHORT");
}
BOOST_AUTO_TEST_CASE(testWithConversion8) {
  testWithConversion<int8_t>("FRAC_CHAR");
}

BOOST_AUTO_TEST_CASE(testReadWriteToDMARegion) {
  boost::shared_ptr<RegisterInfoMap> registerMap = MapFileParser().parse(MAP_FILE_NAME);
  // open a dummy device with the sequence map file
  BackendFactory::getInstance().setDMapFilePath(DMAP_FILE_NAME);
  Device device;
  device.open(DEVICE_ALIAS);

  SequenceInfo sequenceInfo;
  registerMap->getRegisterInfo( MULTIPLEXED_SEQUENCE_PREFIX + "DMA", sequenceInfo, TEST_MODULE_NAME);


  std::vector<int16_t> ioBuffer( sequenceInfo.nBytes / sizeof(int16_t) );

  for(size_t i = 0; i < ioBuffer.size(); ++i) {
    ioBuffer[i]=i;
  }

  device.writeArea(sequenceInfo.address, reinterpret_cast<int32_t*>( &(ioBuffer[0]) ), sequenceInfo.nBytes, sequenceInfo.bar);

  TwoDRegisterAccessor<double> deMultiplexer = device.getTwoDRegisterAccessor<double>(TEST_MODULE_PATH/"DMA");
  deMultiplexer.read();

  int j=0;
  for(size_t i=0; i< 4; ++i) {
    for(size_t sequenceIndex=0; sequenceIndex<16; ++sequenceIndex) {
      BOOST_CHECK( deMultiplexer[sequenceIndex][i] == 4 * j++ );
    }
  }

}

BOOST_AUTO_TEST_CASE(testMixed) {

  // open a dummy device with the sequence map file
  BackendFactory::getInstance().setDMapFilePath(DMAP_FILE_NAME);
  Device device;
  device.open(DEVICE_MIXED_ALIAS);

  TwoDRegisterAccessor<double> myMixedData = device.getTwoDRegisterAccessor<double>("APP0/DAQ0_BAM");
  BufferingRegisterAccessor<int32_t> myRawData = device.getBufferingRegisterAccessor<int32_t>("APP0/AREA_MULTIPLEXED_SEQUENCE_DAQ0_BAM",0,0,true);

  BOOST_CHECK(myMixedData.getNumberOfDataSequences() == 17);
  BOOST_CHECK(myMixedData.getNumberOfSamples() == 372);
  BOOST_CHECK(myMixedData[0].size() == 372);

  myMixedData[0][0] = -24673; //1001 1111 1001 1111
  myMixedData[1][0] = -13724; //1100 1010 0110 0100
  myMixedData[2][0] =  130495;
  myMixedData[3][0] =  513;
  myMixedData[4][0] =  1027;
  myMixedData[5][0] = -56.4;
  myMixedData[6][0] =  78;
  myMixedData[7][0] =  45.2;
  myMixedData[8][0] = -23.9;
  myMixedData[9][0] =  61.3;
  myMixedData[10][0] = -12;

  myMixedData.write();

  myRawData.read();
  BOOST_CHECK(myRawData[0] == -899375201);
  BOOST_CHECK(myRawData[1] ==  130495);
  BOOST_CHECK(myRawData[2] ==  67305985);
  BOOST_CHECK(myRawData[3] ==  5112008);
  BOOST_CHECK(myRawData[4] == -197269459);

  myMixedData.read();

  BOOST_CHECK(myMixedData[0][0] == -24673);
  BOOST_CHECK(myMixedData[1][0] == -13724);
  BOOST_CHECK(myMixedData[2][0] ==  130495);
  BOOST_CHECK(myMixedData[3][0] ==  513);
  BOOST_CHECK(myMixedData[4][0] ==  1027);
  BOOST_CHECK(myMixedData[5][0] == -56);
  BOOST_CHECK(myMixedData[6][0] ==  78);
  BOOST_CHECK(myMixedData[7][0] ==  45);
  BOOST_CHECK(myMixedData[8][0] == -24);
  BOOST_CHECK(myMixedData[9][0] ==  61);
  BOOST_CHECK(myMixedData[10][0] == -12);
}


BOOST_AUTO_TEST_CASE(testNumberOfSequencesDetected) {
  boost::shared_ptr<RegisterInfoMap> registerMap = MapFileParser().parse(MAP_FILE_NAME);
  // open a dummy device with the sequence map file
    BackendFactory::getInstance().setDMapFilePath(DMAP_FILE_NAME);
  Device device;
  device.open(DEVICE_ALIAS);

  TwoDRegisterAccessor<double> deMuxedData = device.getTwoDRegisterAccessor<double>(TEST_MODULE_PATH/"FRAC_INT");

  BOOST_CHECK(deMuxedData.getNumberOfDataSequences() == 3);

}



BOOST_AUTO_TEST_CASE(testCompatibilityLayer) {

  mtca4u::BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
  Device device;
  device.open("PCIE3");

  boost::shared_ptr< mtca4u::MultiplexedDataAccessor<unsigned int> > acc =
      device.getCustomAccessor< mtca4u::MultiplexedDataAccessor<unsigned int> >("NODMA","TEST");

  BOOST_CHECK_THROW( acc->getFixedPointConverter(), DeviceException );

  BOOST_CHECK(acc->getNumberOfDataSequences() == 16);
  BOOST_CHECK((*acc)[0].size() == 4);

  acc->read();
  for(unsigned int i=0; i<acc->getNumberOfDataSequences(); i++) {
    for(unsigned int k=0; k<(*acc)[i].size(); k++) {
      (*acc)[i][k] = i+3*k;
    }
  }

  // non blocking read currently throws in the underlying layer, not much to test
  BOOST_CHECK_THROW( acc->readNonBlocking(), DeviceException );


  acc->write();

  for(unsigned int i=0; i<acc->getNumberOfDataSequences(); i++) {
    for(unsigned int k=0; k<(*acc)[i].size(); k++) {
      (*acc)[i][k] = 0;
    }
  }

  acc->read();

  for(unsigned int i=0; i<acc->getNumberOfDataSequences(); i++) {
    for(unsigned int k=0; k<(*acc)[i].size(); k++) {
      BOOST_CHECK( (*acc)[i][k] == i+3*k );
    }
  }

}



BOOST_AUTO_TEST_SUITE_END()
