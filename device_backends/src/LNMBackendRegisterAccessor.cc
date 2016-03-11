/*
 * LNMBackendRegisterAccessor.cc
 *
 *  Created on: Feb 10, 2016
 *      Author: Martin Hierholzer
 */

#include "LNMBackendRegisterAccessor.h"
#include "DeviceException.h"

namespace mtca4u {

  /********************************************************************************************************************/
  LNMBackendRegisterAccessor::LNMBackendRegisterAccessor(
        boost::shared_ptr<RegisterAccessor> targetAccessor, unsigned int firstIndex, unsigned int length)
  : RegisterAccessor(boost::shared_ptr<mtca4u::DeviceBackend>()),
    _accessor(targetAccessor),
    _firstIndex(firstIndex),
    _length(length)
  {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(read_impl);
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(write_impl);
  }

  /********************************************************************************************************************/

  void LNMBackendRegisterAccessor::readRaw(int32_t *data, size_t dataSize,
      uint32_t addRegOffset) const {
    _accessor->readRaw(data, dataSize, addRegOffset + sizeof(int32_t)*_firstIndex);
  }

  /********************************************************************************************************************/

  void LNMBackendRegisterAccessor::writeRaw(int32_t const *data, size_t dataSize,
      uint32_t addRegOffset) {
    _accessor->writeRaw(data, dataSize, addRegOffset + sizeof(int32_t)*_firstIndex);
  }

  /********************************************************************************************************************/

  RegisterInfoMap::RegisterInfo const &LNMBackendRegisterAccessor::getRegisterInfo() const {
    throw DeviceException("getRegisterInfo() is not possible with logical name mapping backend accessors.",
        DeviceException::NOT_IMPLEMENTED);
  }

  /********************************************************************************************************************/

  FixedPointConverter const &LNMBackendRegisterAccessor::getFixedPointConverter() const {
    return _accessor->getFixedPointConverter();
  }

  /********************************************************************************************************************/

  unsigned int LNMBackendRegisterAccessor::getNumberOfElements() const {
    return _length;
  }

  /********************************************************************************************************************/

  template <typename ConvertedDataType>
  void LNMBackendRegisterAccessor::read_impl(ConvertedDataType *convertedData, size_t nWords, uint32_t wordOffsetInRegister) const {
    ConvertedDataType *data = reinterpret_cast<ConvertedDataType*>(convertedData);
    _accessor->read(data,nWords,wordOffsetInRegister + _firstIndex);
  }

  /********************************************************************************************************************/

  template <typename ConvertedDataType>
  void LNMBackendRegisterAccessor::write_impl(const ConvertedDataType *convertedData, size_t nWords, uint32_t wordOffsetInRegister) {
    const ConvertedDataType *data = reinterpret_cast<const ConvertedDataType*>(convertedData);
    _accessor->write(data,nWords,wordOffsetInRegister + _firstIndex);
  }

} // namespace mtca4u
