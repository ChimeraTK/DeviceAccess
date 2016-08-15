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
      auto itsrc = _rawAccessor->begin(_startAddress);
      for(auto itdst = NDRegisterAccessor<int32_t>::buffer_2D[0].begin();
               itdst != NDRegisterAccessor<int32_t>::buffer_2D[0].end();
             ++itdst) {
        *itdst = _fixedPointConverter.toCooked<int32_t>(*itsrc);
        ++itsrc;
      }
    }
    else {
      NDRegisterAccessor<int32_t>::buffer_2D[0].swap(_rawAccessor->rawDataBuffer);
    }
  }

  template<>
  void NumericAddressedBackendRegisterAccessor<int32_t>::preWrite() {
    if(!isRaw) {
      auto itsrc = _rawAccessor->begin(_startAddress);
      for(auto itdst = NDRegisterAccessor<int32_t>::buffer_2D[0].begin();
               itdst != NDRegisterAccessor<int32_t>::buffer_2D[0].end();
             ++itdst) {
        *itsrc = _fixedPointConverter.toRaw<int32_t>(*itdst);
        ++itsrc;
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
