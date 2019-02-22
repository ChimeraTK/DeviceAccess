/*
 * NumericAddressedBackendRegisterAccessor.cc
 *
 *  Created on: Aug 12, 2016
 *      Author: Martin Hierholzer
 */

#include "NumericAddressedBackendRegisterAccessor.h"

namespace ChimeraTK {

  template<>
  void NumericAddressedBackendRegisterAccessor<int32_t, FixedPointConverter>::doPostRead() {
    if(!isRaw) {
      auto itsrc = _rawAccessor->begin(_startAddress);
      for(auto itdst = NDRegisterAccessor<int32_t>::buffer_2D[0].begin();
          itdst != NDRegisterAccessor<int32_t>::buffer_2D[0].end();
          ++itdst) {
        *itdst = _dataConverter.toCooked<int32_t>(*itsrc);
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
        memcpy(&(*itdst), &(*itsrc), _numberOfWords * sizeof(int32_t));
      }
    }
    SyncNDRegisterAccessor<int32_t>::doPostRead();
  }

  template<>
  void NumericAddressedBackendRegisterAccessor<int32_t, FixedPointConverter>::doPreWrite() {
    if(!isRaw) {
      auto itdst = _rawAccessor->begin(_startAddress);
      for(auto itsrc = NDRegisterAccessor<int32_t>::buffer_2D[0].begin();
          itsrc != NDRegisterAccessor<int32_t>::buffer_2D[0].end();
          ++itsrc) {
        *itdst = _dataConverter.toRaw<int32_t>(*itsrc);
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
        memcpy(&(*itdst), &(*itsrc), _numberOfWords * sizeof(int32_t));
      }
    }
  }

  template<>
  void NumericAddressedBackendRegisterAccessor<int32_t, FixedPointConverter>::doPostWrite() {
    if(isRaw) {
      if(!_rawAccessor->isShared) {
        NDRegisterAccessor<int32_t>::buffer_2D[0].swap(_rawAccessor->rawDataBuffer);
      }
    }
  }

  namespace detail {
    template<>
    FixedPointConverter createDataConverter<FixedPointConverter>(boost::shared_ptr<RegisterInfoMap::RegisterInfo>
            registerInfo) {
      return FixedPointConverter(
          registerInfo->name, registerInfo->width, registerInfo->nFractionalBits, registerInfo->signedFlag);
    }

    template<>
    IEEE754_SingleConverter createDataConverter<IEEE754_SingleConverter>(
        boost::shared_ptr<RegisterInfoMap::RegisterInfo> /*registerInfo*/) {
      return IEEE754_SingleConverter();
    }

  } // namespace detail

  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendRegisterAccessor, FixedPointConverter);
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendRegisterAccessor, IEEE754_SingleConverter);

} /* namespace ChimeraTK */
