#include "FakeDevice.h"
#include "DummyDevice.h"
#include <cmath>

#include "MappedDevice.h"
#include "PcieDevice.h"

// Not so nice: macro code does not have code coverage report. Make sure you test everything!
// Check that nWords is not 0. The readReg command would read the whole register, which
// will the raw buffer size of 0.
#define MTCA4U_DEVMAP_READ( DEVICE_TYPE, CONVERTED_DATA_TYPE, ROUNDING_COMMAND ) \
template<> template<>\
void MappedDevice<DEVICE_TYPE>::RegisterAccessor::read(CONVERTED_DATA_TYPE * convertedData, size_t nWords,\
					   uint32_t wordOffsetInRegister) const { \
  if (nWords==0){\
    return;\
  }\
  \
  std::vector<int32_t> rawDataBuffer(nWords);\
  readRaw(&(rawDataBuffer[0]), nWords*sizeof(int32_t), wordOffsetInRegister*sizeof(int32_t));\
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
  MTCA4U_DEVMAP_ALL_TYPES_FOR_DEVTYPE( PcieDevice )
  MTCA4U_DEVMAP_ALL_TYPES_FOR_DEVTYPE( BaseDevice )
  MTCA4U_DEVMAP_ALL_TYPES_FOR_DEVTYPE( FakeDevice )
  MTCA4U_DEVMAP_ALL_TYPES_FOR_DEVTYPE( DummyDevice )

  /** Specialisation of openDev to be able to instantiate devMap<BaseDevice>.
   *  To be removed when the templatisation of devMap is removed.
   */
  template<>
  void MappedDevice<BaseDevice>::open(const std::string & /*_devFileName*/, const std::string& /*_mapFileName*/,
				int /*_perm*/, DeviceConfigBase* /*_pConfig*/)
  {
    throw MappedDeviceException(std::string("You cannot directly open an instance of BaseDevice!") +
		   " Use openDev(ptrdev ioDevice, ptrmapFile registerMapping) " +
		   " with an implementation like devPCIe as ioDevice.", MappedDeviceException::EX_CANNOT_OPEN_DEVBASE);
  }
}// namespace mtca4u
