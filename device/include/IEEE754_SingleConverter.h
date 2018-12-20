#ifndef CHIMERATK_IEEE754_SINGLE_CONVERTER_H
#define CHIMERATK_IEEE754_SINGLE_CONVERTER_H

#include <boost/numeric/conversion/cast.hpp>
#include "Exception.h"

namespace ChimeraTK{

  /** Needs to have the same interface as FixedPointConverter, except for the constructor.
   *  Converter for IEEE754 single precision (32bit) floating point.
   */
  struct IEEE754_SingleConverter{

    template<typename CookedType>
    CookedType toCooked(uint32_t rawValue) const;

    template<typename CookedType>
      uint32_t toRaw(CookedType cookedValue) const{
      return 0;
    }

    /** define round type for the boost::numeric::converter */
    template<class S>
    struct Round {
      static S nearbyint ( S s ){
        return round(s);
      }
      
      typedef boost::mpl::integral_c<std::float_round_style,std::round_to_nearest> round_style;
    };

  };

  template<typename CookedType>
  CookedType  IEEE754_SingleConverter::toCooked(uint32_t rawValue) const{
    //Step 1: convert the raw data to the "generic" representation in the CPU: float
    void * warningAvoider = &rawValue;
    float genericRepresentation = *(reinterpret_cast<float *>(warningAvoider));

    //Step 2: convert the float to the cooked type
    CookedType cooked;
    
    typedef boost::numeric::converter<CookedType, float, boost::numeric::conversion_traits<CookedType, float>,
      boost::numeric::def_overflow_handler,
      Round<float> >   converter;

    try{
      cooked = converter::convert(genericRepresentation);
    }catch( boost::numeric::positive_overflow & ) {
      throw ChimeraTK::runtime_error("positive overflow"); //+_variableName);
    }catch( boost::numeric::negative_overflow & ) {
      throw ChimeraTK::runtime_error("negative overflow"); //+_variableName);
    }

    return cooked;
  }
  

}// namespace ChimeraTK

#endif // CHIMERATK_IEEE754_SINGLE_CONVERTER_H
