/*
 * NumericAddressedBackendRegisterAccessor.cc
 *
 *  Created on: Aug 12, 2016
 *      Author: Martin Hierholzer
 */

#include "NumericAddressedBackendRegisterAccessor.h"

namespace ChimeraTK {

  template<>
  void NumericAddressedBackendRegisterAccessor<int32_t>::doPostRead() {
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
      if(!_rawAccessor->isShared) {
        NDRegisterAccessor<int32_t>::buffer_2D[0].swap(_rawAccessor->rawDataBuffer);
      }
      else {
        auto itsrc = _rawAccessor->begin(_startAddress);
        auto itdst = NDRegisterAccessor<int32_t>::buffer_2D[0].begin();
        memcpy(&(*itdst), &(*itsrc), _numberOfWords*sizeof(int32_t));
      }
    }
    SyncNDRegisterAccessor<int32_t>::doPostRead();
  }

  template<>
  void NumericAddressedBackendRegisterAccessor<int32_t>::doPreWrite() {
    if(!isRaw) {
      auto itdst = _rawAccessor->begin(_startAddress);
      for(auto itsrc = NDRegisterAccessor<int32_t>::buffer_2D[0].begin();
               itsrc != NDRegisterAccessor<int32_t>::buffer_2D[0].end();
             ++itsrc) {
        *itdst = _fixedPointConverter.toRaw<int32_t>(*itsrc);
        ++itdst;
      }
    }
    else {
      if(!_rawAccessor->isShared) {
        NDRegisterAccessor<int32_t>::buffer_2D[0].swap(_rawAccessor->rawDataBuffer);
      }
      else {
        auto itdst = _rawAccessor->begin(_startAddress);
        auto itsrc = NDRegisterAccessor<int32_t>::buffer_2D[0].begin();
        memcpy(&(*itdst), &(*itsrc), _numberOfWords*sizeof(int32_t));
      }
    }
  }

  template<>
  void NumericAddressedBackendRegisterAccessor<int32_t>::doPostWrite() {
    if(isRaw) {
      if(!_rawAccessor->isShared) {
        NDRegisterAccessor<int32_t>::buffer_2D[0].swap(_rawAccessor->rawDataBuffer);
      }
    }
  }

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendRegisterAccessor);
  
} /* namespace ChimeraTK */
