#include "devMap.h"
#include "devPCIE.h"
#include "devFake.h"
#include "DummyDevice.h"
#include <cmath>

// not so nice: macro code does not have code coverage report. Make sure you test everything!
#define MTCA4U_DEVMAP_READ( DEVICE_TYPE, CONVERTED_DATA_TYPE, ROUNDING_COMMAND ) \
template<> template<>\
void devMap<DEVICE_TYPE>::RegisterAccessor::read(CONVERTED_DATA_TYPE * convertedData, size_t nWords,\
					   uint32_t addRegOffset) const { \
  if (nWords==0){\
    return;\
  }\
  \
  std::vector<int32_t> rawDataBuffer(nWords);\
  readReg(&(rawDataBuffer[0]), nWords*sizeof(int32_t), addRegOffset);\
  \
  for(size_t i=0; i < nWords; ++i){\
      convertedData[i] = static_cast<CONVERTED_DATA_TYPE>(ROUNDING_COMMAND(_fixedPointConverter.toDouble(rawDataBuffer[i]))); \
  }\
}

//Trick for floating point types: Empty arguments are not allowed in C++98 (only from C++11).
//Just insert another static cast, double casting to the same type should be a no-op and optimised by the compiler.
#define MTCA4U_DEVMAP_ALL_TYPES_FOR_DEVTYPE( DEVICE_TYPE )\
MTCA4U_DEVMAP_READ( DEVICE_TYPE, int32_t, round )  \
MTCA4U_DEVMAP_READ( DEVICE_TYPE, uint32_t, round  )\
MTCA4U_DEVMAP_READ( DEVICE_TYPE, int16_t, round  )\
MTCA4U_DEVMAP_READ( DEVICE_TYPE, uint16_t, round  )\
MTCA4U_DEVMAP_READ( DEVICE_TYPE, int8_t, round )\
MTCA4U_DEVMAP_READ( DEVICE_TYPE, uint8_t, round  )\
MTCA4U_DEVMAP_READ( DEVICE_TYPE, float , static_cast<float>)\
MTCA4U_DEVMAP_READ( DEVICE_TYPE, double, static_cast<double> )\


namespace mtca4u{

  MTCA4U_DEVMAP_ALL_TYPES_FOR_DEVTYPE( devPCIE )
  MTCA4U_DEVMAP_ALL_TYPES_FOR_DEVTYPE( devBase )
  MTCA4U_DEVMAP_ALL_TYPES_FOR_DEVTYPE( devFake )
  MTCA4U_DEVMAP_ALL_TYPES_FOR_DEVTYPE( DummyDevice )
}// namespace mtca4u
