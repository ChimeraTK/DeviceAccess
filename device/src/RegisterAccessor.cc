/*
 * RegisterAccessor.cc - Non-buffering accessor for device registers
 */

#include "RegisterAccessor.h"
#include "Device.h"

#include <typeinfo>

namespace mtca4u {

  RegisterAccessor::RegisterAccessor(boost::shared_ptr<DeviceBackend> deviceBackendPointer,
      const RegisterPath &registerPathName)
    : _registerPathName(registerPathName), _dev(deviceBackendPointer),
    _registerInfo(_dev->getRegisterCatalogue().getRegister(registerPathName)){
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
    NDAccessorPtr<int> & accessor = boost::fusion::at_key<int>(_convertingAccessors.table);
    if (! accessor){
      std::cout << "converting accessor for type " <<
	typeid(int).name() << " not initalised yet" << std::endl;
      accessor = _dev->getRegisterAccessor<int>(_registerPathName,
						_registerInfo->getNumberOfElements(),
						0, false); // 0 offset, not raw
    }
    if (nWords == 0){
	return;
    }
    if (nWords+wordOffsetInRegister > accessor->accessChannel(0).size() ){
      std::cout << "trying to access too large register. new impl" << std::endl; //keep this to check what is executed when debugging 
      throw DeviceException("RegisterAccessor::read Error: reading over the end of register",DeviceException::WRONG_PARAMETER);
    }
    accessor->read();
    // copy data to target buffer
    memcpy(convertedData, accessor->accessChannel(0).data() + wordOffsetInRegister, nWords*sizeof(int));
  }


}
