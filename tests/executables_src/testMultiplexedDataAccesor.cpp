
#define BOOST_TEST_MODULE SequenceDeMultiplexerTest
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "MultiplexedDataAccessor.h"
#include "AddressBasedMuxedDataAccessor.h"
#include "DummyBackend.h"
#include "MapFileParser.h"
#include <iostream>

#include <sstream>

using namespace mtca4u;

static const std::string MAP_FILE_NAME("sequences.map");
static const std::string BAM_MAP_FILE("bam_fmc25_r1225.mapp");
static const std::string TEST_MODULE_NAME("TEST");
static const std::string INVALID_MODULE_NAME("INVALID");

BOOST_AUTO_TEST_SUITE( SequenceDeMultiplexerTestSuite )

BOOST_AUTO_TEST_CASE( testConstructor ){
  boost::shared_ptr< DeviceBackend > ioDevice( new DummyBackend(MAP_FILE_NAME) );
  MixedTypeMuxedDataAccessor<double> deMultiplexer("FRAC_INT",TEST_MODULE_NAME,ioDevice);
  BOOST_CHECK( deMultiplexer[0].size() == 5 );
}

// test the de-multiplexing itself, with 'identity' fixed point conversion
template <class SequenceWordType>
void testDeMultiplexing(std::string areaName) {

    // open a dummy device with the sequence map file
    boost::shared_ptr< DeviceBackend >  ioDevice( new DummyBackend(MAP_FILE_NAME) );
    ioDevice->open();

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

    ioDevice->write(sequenceInfo.bar, sequenceInfo.address, reinterpret_cast<int32_t*>( &(ioBuffer[0]) ), sequenceInfo.nBytes);

    MixedTypeMuxedDataAccessor< SequenceWordType > deMultiplexer(areaName, TEST_MODULE_NAME, ioDevice);

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

    deMultiplexer.write();
    ioDevice->read( sequenceInfo.bar, sequenceInfo.address, reinterpret_cast<int32_t*>( &(ioBuffer[0]) ),
        sequenceInfo.nBytes );

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
    // open a dummy device with the sequence map file
    boost::shared_ptr< DeviceBackend >  ioDevice( new DummyBackend(MAP_FILE_NAME) );
    ioDevice->open();

    //get the sequence info from the map file
    boost::shared_ptr<RegisterInfoMap> registerMap = MapFileParser().parse(MAP_FILE_NAME);
    SequenceInfo sequenceInfo;
    registerMap->getRegisterInfo(MULTIPLEXED_SEQUENCE_PREFIX + multiplexedSequenceName, sequenceInfo, TEST_MODULE_NAME);

    std::vector< SequenceWordType > ioBuffer( sequenceInfo.nBytes/sizeof(SequenceWordType) );

    for (size_t i=0; i < ioBuffer.size(); ++i) {
      ioBuffer[i] = i;
    }

    ioDevice->write(sequenceInfo.bar, sequenceInfo.address, reinterpret_cast<int32_t*>( &(ioBuffer[0]) ), sequenceInfo.nBytes);

    boost::shared_ptr< MultiplexedDataAccessor<float> > deMultiplexer =
        ioDevice->getRegisterAccessor2D<float>(multiplexedSequenceName,TEST_MODULE_NAME);
    deMultiplexer->read();

    BOOST_CHECK( (*deMultiplexer)[0][0] == 0 );
    BOOST_CHECK( (*deMultiplexer)[1][0] == 0.25 ); // 1 with 2 frac bits
    BOOST_CHECK( (*deMultiplexer)[2][0] == 0.25 ); // 2 with 3 frac bits
    BOOST_CHECK( (*deMultiplexer)[0][1] == 1.5 ); // 3 with 1 frac bits
    BOOST_CHECK( (*deMultiplexer)[1][1] == 1 ); // 4 with 2 frac bits
    BOOST_CHECK( (*deMultiplexer)[2][1] == 0.625 ); // 5 with 3 frac bits
    BOOST_CHECK( (*deMultiplexer)[0][2] == 3. ); // 6 with 1 frac bits
    BOOST_CHECK( (*deMultiplexer)[1][2] == 1.75 ); // 7 with 2 frac bits
    BOOST_CHECK( (*deMultiplexer)[2][2] == 1. ); // 8 with 3 frac bits
    BOOST_CHECK( (*deMultiplexer)[0][3] == 4.5 ); // 9 with 1 frac bits
    BOOST_CHECK( (*deMultiplexer)[1][3] == 2.5 ); // 10 with 2 frac bits
    BOOST_CHECK( (*deMultiplexer)[2][3] == 1.375 ); // 11 with 3 frac bits
    BOOST_CHECK( (*deMultiplexer)[0][4] == 6. ); // 12 with 1 frac bits
    BOOST_CHECK( (*deMultiplexer)[1][4] == 3.25 ); // 13 with 2 frac bits
    BOOST_CHECK( (*deMultiplexer)[2][4] == 1.75 ); // 14 with 3 frac bits

    for(size_t sequenceIndex=0; sequenceIndex<3; ++sequenceIndex) {
      for(size_t i=0; i< 5; ++i) {
        (*deMultiplexer)[sequenceIndex][i]+=1.;
      }
    }

    deMultiplexer->write();
    ioDevice->read(sequenceInfo.bar, sequenceInfo.address, reinterpret_cast<int32_t*>( &(ioBuffer[0]) ), sequenceInfo.nBytes );

    for(size_t i=0; i < 15; ++i) {
      // with i%3+1 fractional bits the added floating point value of 1
      // corresponds to 2^(i%3+1) in fixed point represetation
      int addedValue = 1 << (i%3+1);
      std::stringstream message;
      message << "ioBuffer["<<i<< "] is " << ioBuffer[i] << ", expected " << i+addedValue;
      BOOST_CHECK_MESSAGE( ioBuffer[i] == static_cast< SequenceWordType>(i+addedValue) , message.str());
    }

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

BOOST_AUTO_TEST_CASE(testFactoryFunction) {
  boost::shared_ptr<RegisterInfoMap> registerMap = MapFileParser().parse("invalidSequences.map");
  boost::shared_ptr< DeviceBackend > ioDevice( new DummyBackend("invalidSequences.map") );

  try {
    ioDevice->getRegisterAccessor2D<double>("NO_WORDS",INVALID_MODULE_NAME);
    // in a sucessful test (which is required for the code coverage report)
    // the following line is not executed. Exclude it from the lcov report
    BOOST_ERROR( "createInstance did not throw for NO_WORDS" ); //LCOV_EXCL_LINE
  }
  catch(MultiplexedDataAccessorException &e) {
    BOOST_CHECK( e.getID() == MultiplexedDataAccessorException::EMPTY_AREA );
  }

  try {
    ioDevice->getRegisterAccessor2D<double>("WRONG_SIZE",INVALID_MODULE_NAME);
    BOOST_ERROR( "createInstance did not throw for WRONG_SIZE" ); //LCOV_EXCL_LINE
  }
  catch(MultiplexedDataAccessorException &e) {
    BOOST_CHECK( e.getID() == MultiplexedDataAccessorException::INVALID_WORD_SIZE );
  }

  try{
    ioDevice->getRegisterAccessor2D<double>("WRONG_NELEMENTS",INVALID_MODULE_NAME);
    BOOST_ERROR( "createInstance did not throw for WRONG_NELEMENTS" ); //LCOV_EXCL_LINE
  }
  catch(MultiplexedDataAccessorException &e) {
    BOOST_CHECK( e.getID() == MultiplexedDataAccessorException::INVALID_N_ELEMENTS );
  }

  BOOST_CHECK_THROW( ioDevice->getRegisterAccessor2D<double>("DOES_NOT_EXIST",INVALID_MODULE_NAME), MapFileException );
}

BOOST_AUTO_TEST_CASE(testReadWriteToDMARegion) {
  boost::shared_ptr<RegisterInfoMap> registerMap = MapFileParser().parse(MAP_FILE_NAME);
  // open a dummy device with the sequence map file
  boost::shared_ptr< DeviceBackend >  ioDevice( new DummyBackend(MAP_FILE_NAME) );
  ioDevice->open();

  SequenceInfo sequenceInfo;
  registerMap->getRegisterInfo( MULTIPLEXED_SEQUENCE_PREFIX + "DMA", sequenceInfo, TEST_MODULE_NAME);


  std::vector<int16_t> ioBuffer( sequenceInfo.nBytes / sizeof(int16_t) );

  for(size_t i = 0; i < ioBuffer.size(); ++i) {
    ioBuffer[i]=i;
  }

  ioDevice->write(sequenceInfo.bar, sequenceInfo.address, reinterpret_cast<int32_t*>( &(ioBuffer[0]) ), sequenceInfo.nBytes);

  boost::shared_ptr< MultiplexedDataAccessor<double> > deMultiplexer =
      ioDevice->getRegisterAccessor2D<double>("DMA",TEST_MODULE_NAME);
  deMultiplexer->read();

  int j=0;
  for(size_t i=0; i< 4; ++i) {
    for(size_t sequenceIndex=0; sequenceIndex<16; ++sequenceIndex) {
      BOOST_CHECK( (*deMultiplexer)[sequenceIndex][i] == 4 * j++ );
    }
  }

  //BOOST_CHECK_THROW( deMultiplexer->write(), NotImplementedException );


}

/*BOOST_AUTO_TEST_CASE( testMixed ){
  boost::shared_ptr<RegisterInfoMap> registerMap = mapFileParser().parse(MAP_FILE_NAME);
  boost::shared_ptr< DeviceBackend > ioDevice( new DummyBackend );
  ioDevice->open( MAP_FILE_NAME );

  BOOST_CHECK_THROW( MultiplexedDataAccessor<double>::createInstance( "MIXED",
								    TEST_MODULE_NAME,
								    ioDevice,
								    registerMap ),
		     NotImplementedException );
}*/

BOOST_AUTO_TEST_CASE(testMixed) {
  // open a dummy device with the sequence map file
  boost::shared_ptr<DeviceBackend> ioDevice( new DummyBackend(BAM_MAP_FILE) );
  ioDevice->open();
  MixedTypeMuxedDataAccessor<double> myMixedData("DAQ0_BAM", "APP0", ioDevice);

  MixedTypeTest<double> myTest(&myMixedData);
  BOOST_CHECK(myTest.getSizeOneBlock() == 11);
  BOOST_CHECK(myMixedData.getNumberOfDataSequences() == 17);
  BOOST_CHECK(myTest.getConvertersSize() == 17);
  BOOST_CHECK(myTest.getNBlock() == 372);

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

  BOOST_CHECK(myTest.getIOBUffer(0) == -899375201);
  BOOST_CHECK(myTest.getIOBUffer(1) ==  130495);
  BOOST_CHECK(myTest.getIOBUffer(2) ==  67305985);
  BOOST_CHECK(myTest.getIOBUffer(3) ==  5112008);
  BOOST_CHECK(myTest.getIOBUffer(4) == -197269459);

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
  boost::shared_ptr< DeviceBackend >  ioDevice( new DummyBackend(MAP_FILE_NAME) );
  ioDevice->open();

  boost::shared_ptr< MultiplexedDataAccessor<double> > deMuxedData =
      ioDevice->getRegisterAccessor2D<double>("FRAC_INT", TEST_MODULE_NAME);

  BOOST_CHECK(deMuxedData->getNumberOfDataSequences() == 3);

}

BOOST_AUTO_TEST_SUITE_END()
