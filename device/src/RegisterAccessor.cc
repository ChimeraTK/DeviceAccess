/*
 * RegisterAccessor.cc - Non-buffering accessor for device registers
 */

#include "RegisterAccessor.h"
#include "Device.h"

namespace mtca4u {

  RegisterAccessor::RegisterAccessor(boost::shared_ptr<DeviceBackend> deviceBackendPointer,
      const RegisterPath &registerPathName)
  : _registerPathName(registerPathName), _dev(deviceBackendPointer){
    auto registerInfo =_dev->getRegisterCatalogue().getRegister(registerPathName) ;
    _intAccessor = _dev->getRegisterAccessor<int>(registerPathName, registerInfo->getNumberOfElements(), 0, false);
  }

  RegisterAccessor::~RegisterAccessor() {}

  /********************************************************************************************************************/

  void RegisterAccessor::readDMA(int32_t *data, size_t dataSize, uint32_t addRegOffset) const {
    readRaw(data,dataSize,addRegOffset);
  }

  /********************************************************************************************************************/

  void RegisterAccessor::writeDMA(int32_t const *data, size_t dataSize, uint32_t addRegOffset) {   // LCOV_EXCL_LINE
    writeRaw(data,dataSize,addRegOffset);   // LCOV_EXCL_LINE
  }   // LCOV_EXCL_LINE

  template <>
  void RegisterAccessor::read<int>(int *convertedData, size_t nWords, uint32_t wordOffsetInRegister) const{
    // if (! _intAccessor) create it
    if ( (nWords == 0)||(nWords+wordOffsetInRegister > _intAccessor->accessChannel(0).size()) ){
	return;
    }
    _intAccessor->read();
    // copy data to target buffer
    memcpy(convertedData, _intAccessor->accessChannel(0).data() + wordOffsetInRegister, nWords*sizeof(int));
  }


}
