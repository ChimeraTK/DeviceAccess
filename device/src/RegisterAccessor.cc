/*
 * RegisterAccessor.cc - Non-buffering accessor for device registers
 */

#include "RegisterAccessor.h"
#include "Device.h"

#include <typeinfo>

namespace ChimeraTK {

  RegisterAccessor::RegisterAccessor(boost::shared_ptr<DeviceBackend> deviceBackendPointer,
      const RegisterPath &registerPathName)
    : _registerPathName(registerPathName), _backend(deviceBackendPointer),
    _registerInfo(_backend->getRegisterCatalogue().getRegister(registerPathName)){
    std::cerr << "*************************************************************************************************" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Class RegisterAccessor is deprecated and will be removed soon.                              **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Use OneDRegisterAccessor instead!                                                           **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "*************************************************************************************************" << std::endl;// LCOV_EXCL_LINE
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

}
