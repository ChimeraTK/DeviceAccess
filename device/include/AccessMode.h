/*
 * AccessMode.h
 *
 *  Created on: May 11, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_ACCESS_MODE_H
#define MTCA4U_ACCESS_MODE_H

#include <set>

#include "DeviceException.h"

namespace mtca4u {

  /** Enum type with access mode flags for register accessors. */
  enum class AccessMode {

    /** Raw access: disable any possible conversion from the original hardware data type into the given UserType.
     *  Obtaining the accessor with a UserType unequal to the actual raw data type will fail and throw a
     *  DeviceException with the id EX_WRONG_PARAMETER.
     *
     *  Note: using this flag will make your code intrinsically dependent on the backend type, since the actual
     *  raw data type must be known. */
    raw,

    /** Make any read blocking until new data has arrived since the last read. This flag may not be suppoerted by
     *  all registers (and backends), in which case a DeviceException with the id NOT_IMPLEMENTED will be thrown. */
    wait_for_new_data

  };

  /** */
  class AccessModeFlags {

    public:

      AccessModeFlags(std::set<AccessMode> flags)
      : _flags(flags)
      {
      }

      bool has(AccessMode flag) {
        return ( _flags.count(flag) != 0 );
      }

      bool empty() {
        return ( _flags == std::set<AccessMode>() );
      }

      void checkForUnknownFlags(std::set<AccessMode> knownFlags) {
        for(auto flag : _flags) {
          if(knownFlags.count(flag) == 0) {
            throw DeviceException("Access mode flag "+
                std::to_string(static_cast<typename std::underlying_type<AccessMode>::type>(flag))+
                " is not known by this backend.",
                DeviceException::NOT_IMPLEMENTED);
          }
        }
      }

    private:

      std::set<AccessMode> _flags;
  };

} /* namespace mtca4u */

#endif /* MTCA4U_ACCESS_MODE_H */
