#ifndef MTCA4U_DEVICE_EXCEPTION_H
#define	MTCA4U_DEVICE_EXCEPTION_H

#include "DeviceBackendException.h"

namespace mtca4u{

  /** A class to provide exceptions for Device. */
  class DeviceException : public DeviceBackendException {
    public:

      enum { NOT_IMPLEMENTED = 0,
             EX_WRONG_PARAMETER,
             EX_NOT_OPENED,
             EX_CANNOT_OPEN_DEVICEBACKEND,
             CANNOT_OPEN_MAP_FILE,
             REGISTER_DOES_NOT_EXIST,
             REGISTER_IS_READ_ONLY,
             // for compatibility with MultiplexedDataAccessorException:
             EMPTY_AREA = CANNOT_OPEN_MAP_FILE,
             INVALID_WORD_SIZE = CANNOT_OPEN_MAP_FILE,
             INVALID_N_ELEMENTS = CANNOT_OPEN_MAP_FILE
      };


      DeviceException(const std::string &_exMessage, unsigned int _exID);
      virtual ~DeviceException() throw();
      friend std::ostream& operator<<(std::ostream &os, const DeviceException& e);
    private:

  };

  typedef DeviceException TwoDRegisterAccessorException;
  typedef DeviceException MultiplexedDataAccessorException;

}//namespace mtca4u

#endif	/* MTCA4U_DEVICE_EXCEPTION_H */

