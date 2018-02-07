#ifndef MTCA4U_DEVICE_EXCEPTION_H
#define MTCA4U_DEVICE_EXCEPTION_H

#include "DeviceBackendException.h"

namespace ChimeraTK{

  /** A class to provide exceptions for Device. */
  class DeviceException : public DeviceBackendException {
    public:

      enum {
        /** The function called is not implemented yet, e.g.\ for the used backend.
         *  Inprinciple it can be implemented and might be available in future (in 
         *  contrast to NOT_AVAILABLE, which indicates that a function cannot be 
         *  implemented and will never be available in this context).
         */
        NOT_IMPLEMENTED = 0,

        /** A parameter (function argument, value in a map file etc.) is not valid */
        WRONG_PARAMETER,

        /** The called operation requires an opened device but it is closed */
        NOT_OPENED,

        /** The backend refused to open, e.g.\ due to a connection error with the hardware */
        CANNOT_OPEN_DEVICEBACKEND,

        /** The map file could not be openend or contains errors */
        CANNOT_OPEN_MAP_FILE,

        /** The register specified in the operation does not exist */
        REGISTER_DOES_NOT_EXIST,

        /** A write request was sent to a read-only register */
        REGISTER_IS_READ_ONLY,

        /** The requested accessor is not suitable for the given register (e.g.\ accessor has too low dimension) */
        WRONG_ACCESSOR,

        /** The dmap file path has not been set */
        NO_DMAP_FILE,

        /** There has been an error (logical or parse error) in the dmap file*/
        DMAP_FILE_ERROR,

        /** A function or requested functionality is not available, e.g.\ for a particular backend.
         *  The functionality conceptually does not make sense in this context
         *  and cannot be implemented (in constrast to NOT_IMPLEMENTED, which means the function
         *  has not been implemented yet but might be available in future releases).
         */
        NOT_AVAILABLE,

        /** Deprecated, for compatibility with MultiplexedDataAccessorException (mapfile contains error) */
        EMPTY_AREA = CANNOT_OPEN_MAP_FILE,

        /** Deprecated, for compatibility with MultiplexedDataAccessorException (mapfile contains error) */
        INVALID_WORD_SIZE = CANNOT_OPEN_MAP_FILE,

        /** Deprecated, for compatibility with MultiplexedDataAccessorException (mapfile contains error) */
        INVALID_N_ELEMENTS = CANNOT_OPEN_MAP_FILE,

        /** Deprecated, for compatibility only */
        EX_WRONG_PARAMETER = WRONG_PARAMETER,
        EX_NOT_OPENED = NOT_OPENED,
        EX_CANNOT_OPEN_DEVICEBACKEND = CANNOT_OPEN_DEVICEBACKEND
      };


      DeviceException(const std::string &_exMessage, unsigned int _exID);
      virtual ~DeviceException() throw();
      friend std::ostream& operator<<(std::ostream &os, const DeviceException& e);
    private:

  };

  typedef DeviceException TwoDRegisterAccessorException;
  typedef DeviceException MultiplexedDataAccessorException;

}//namespace ChimeraTK

#endif  /* MTCA4U_DEVICE_EXCEPTION_H */

