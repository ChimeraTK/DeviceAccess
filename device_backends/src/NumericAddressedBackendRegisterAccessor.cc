/*
 * NumericAddressedBackendRegisterAccessor.cc
 *
 *  Created on: Aug 12, 2016
 *      Author: Martin Hierholzer
 */

#include "NumericAddressedBackendRegisterAccessor.h"

namespace mtca4u {

  template<>
  void NumericAddressedBackendRegisterAccessor<int32_t>::postRead() {
    if(!isRaw) {
      for(size_t i=0; i < _numberOfWords; ++i){
        NDRegisterAccessor<int32_t>::buffer_2D[0][i] =
            _fixedPointConverter.toCooked<int32_t>(_rawAccessor->getData(_startAddress+sizeof(int32_t)*i));
      }
    }
    else {
      NDRegisterAccessor<int32_t>::buffer_2D[0].swap(_rawAccessor->rawDataBuffer);
    }
  }

  template<>
  void NumericAddressedBackendRegisterAccessor<int32_t>::preWrite() {
    if(!isRaw) {
      for(size_t i=0; i < _numberOfWords; ++i){
        _rawAccessor->getData(_startAddress+sizeof(int32_t)*i) =
            _fixedPointConverter.toRaw(NDRegisterAccessor<int32_t>::buffer_2D[0][i]);
      }
    }
    else {
      NDRegisterAccessor<int32_t>::buffer_2D[0].swap(_rawAccessor->rawDataBuffer);
    }
  }

  template<>
  void NumericAddressedBackendRegisterAccessor<int32_t>::postWrite() {
    if(isRaw) {
      NDRegisterAccessor<int32_t>::buffer_2D[0].swap(_rawAccessor->rawDataBuffer);
    }
  }

} /* namespace mtca4u */
