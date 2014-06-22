#ifndef MTCA4U_FIXED_POINT_CONVERTER_H
#define MTCA4U_FIXED_POINT_CONVERTER_H
#include <stdint.h>

namespace mtca4u{

  /** The fixed point converter provides conversion functions 
   *  between double and up to 32 bit fixed point (signed or unsigned).
   */
  class FixedPointConverter{
  public:
    /** The constructor defines the conversion factor.
     *  @param nBits The number of total bits must not exceed 32.
     *  @param fractionalBits The numer of fractional bits can range from 
     *    -1024+nBits to 1023-nBits. This is the maximum dynamica range in which
     *    the fixed point number can be
     *    represented by a double without rounding errors.
     *  @param isSigned Flag whether the most significant bit is interpreted as
     *    sign bit. Negative numbers are interpretes as two's complement
     *    number of the respective number of bits 
     *    (i.e. in signed 6 bit, 0 fractional bits 0x3F is -1)
     */
    FixedPointConverter(unsigned int nBits = 32, int fractionalBits = 0,
			bool isSigned=true);

    /** Converstion function from fixed value to double.
     *  There is no consistency check of the input.
     *  In case less than 32 bits are used it is in the responsibility of the
     *  user that the leading bits are 0. Otherwise the results will be wrong.
     */
    double toDouble(uint32_t fixedPointValue) const;

    /** Converstion function from double to fixed point.
     *  This conversion usually will introduce rounding errors due to this limited
     *  resolution of the fixed point number compared to the double.
     *  In case of a high number of fractional bits this can mean that the most
     *  significant digits are cut and only the 'noise' in the less significant bits
     *  is represented.
     */
    uint32_t toFixedPoint(double floatingPointValue) const;

    /** This function allows to change the fixed point parameters after
     *  the object has been created. It will be removed from the interface
     *  once the creating mechanism reading the relevant information from
     *  the xml mapping have been implemented.
     */
    void setParameters(unsigned int nBits = 32, int fractionalBits = 0,
		       bool isSigned=true);

  private:
    unsigned int _nBits; //FIXME: should be const when setParameters is removed
    int _fractionalBits; //FIXME: should be const
    bool _isSigned; //FIXME: should be const

    /// Coefficient containing the multiplication factor described by the 
    /// fractional bits 2^fractionalBits
    double _fractionalBitsCoefficient; //FIXME: should be const

    /// Coefficient containing the inverse multiplication factor described by the 
    /// fractional bits 2^(-fractionalBits). Used to always multiply because this is
    /// faster than division in the floating point unit.
    double _inverseFractionalBitsCoefficient; //FIXME: should be const

    uint32_t _signBitMask; ///< The bit which represents the sign
    uint32_t _usedBitsMask; ///< The bits which are used
    uint32_t _unusedBitsMask; ///< The bits which are not used
  };

}// namespace mtca4u

#endif // MTCA4U_FIXED_POINT_CONVERTER_H
