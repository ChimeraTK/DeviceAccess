#include "devMap.h"
#include "devPCIE.h"
#include "devFake.h"
#include "DummyDevice.h"
#include <cmath>

// for integers we need rounding
#define MTCA4U_DEVMAP_INTEGER_TYPE_READ( DEVICE_TYPE, CONVERTED_DATA_TYPE )\
template<> template<>\
void devMap<DEVICE_TYPE>::regObject::read(CONVERTED_DATA_TYPE * convertedData, size_t nWords,\
					   uint32_t addRegOffset) const { \
  std::vector<int32_t> rawDataBuffer(nWords);\
  readReg(&(rawDataBuffer[0]), nWords*sizeof(int32_t), addRegOffset);\
  \
  for(size_t i=0; i < nWords; ++i){\
    convertedData[i] = static_cast<CONVERTED_DATA_TYPE>(round(_fixedPointConverter.toDouble(rawDataBuffer[i])));\
  }\
}

// for floating point values there is no rounding
#define  MTCA4U_DEVMAP_FLOAT_TYPE_READ( DEVICE_TYPE, CONVERTED_DATA_TYPE )	\
template<> template<>\
void devMap<DEVICE_TYPE>::regObject::read(CONVERTED_DATA_TYPE * convertedData, size_t nWords,\
				uint32_t addRegOffset) const {\
  std::vector<int32_t> rawDataBuffer(nWords);\
  readReg(&(rawDataBuffer[0]), nWords*sizeof(int32_t), addRegOffset);\
  \
  for(size_t i=0; i < nWords; ++i){\
    convertedData[i] = static_cast<CONVERTED_DATA_TYPE>(_fixedPointConverter.toDouble(rawDataBuffer[i]));\
  }\
}

#define MTCA4U_DEVMAP_ALL_TYPES_FOR_DEVTYPE( DEVICE_TYPE )\
MTCA4U_DEVMAP_INTEGER_TYPE_READ( DEVICE_TYPE, int32_t )\
MTCA4U_DEVMAP_INTEGER_TYPE_READ( DEVICE_TYPE, uint32_t )\
MTCA4U_DEVMAP_INTEGER_TYPE_READ( DEVICE_TYPE, int16_t )\
MTCA4U_DEVMAP_INTEGER_TYPE_READ( DEVICE_TYPE, uint16_t )\
MTCA4U_DEVMAP_INTEGER_TYPE_READ( DEVICE_TYPE, int8_t )\
MTCA4U_DEVMAP_INTEGER_TYPE_READ( DEVICE_TYPE, uint8_t )\
MTCA4U_DEVMAP_FLOAT_TYPE_READ( DEVICE_TYPE, float )\
MTCA4U_DEVMAP_FLOAT_TYPE_READ( DEVICE_TYPE, double )\


namespace mtca4u{

  MTCA4U_DEVMAP_ALL_TYPES_FOR_DEVTYPE( devPCIE )
  MTCA4U_DEVMAP_ALL_TYPES_FOR_DEVTYPE( devBase )
  MTCA4U_DEVMAP_ALL_TYPES_FOR_DEVTYPE( devFake )
  MTCA4U_DEVMAP_ALL_TYPES_FOR_DEVTYPE( DummyDevice )
}// namespace mtca4u
