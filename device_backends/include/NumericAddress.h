/*
 * NumericAddress.h
 *
 *  Created on: Mar 22, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_NUMERIC_ADDRESS_H
#define MTCA4U_NUMERIC_ADDRESS_H

#include "RegisterPath.h"

namespace ChimeraTK {
  namespace numeric_address {
  
    /** The numeric_address::BAR constant can be used to directly access registers by numeric addresses, instead of
     *  using register names (e.g. when no map file exists). The address will form a special RegisterPath which can
     *  be used in any place where a register path name is expected, if the backend supports it. The syntax is as
     *  follows:
     *    BAR/<barNumber>/<addressInBytes> -> access 4-byte register in bar <barNumber> at byte offset <addressInBytes>
     *    BAR/<barNumber>/<addressInBytes>*<lengthInBytes> -> access register with arbitrary length of <lengthInBytes>
     */
    static const RegisterPath BAR("#");
  }

}

#endif /* MTCA4U_NUMERIC_ADDRESS_H */
