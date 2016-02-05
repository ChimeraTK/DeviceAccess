#ifndef MTCA4U_DEVICE_EXCEPTION_H
#define	MTCA4U_DEVICE_EXCEPTION_H

#include "DeviceBackendException.h"

namespace mtca4u{
  /** A class to provide exception for class device.
   *
   */
  class DeviceException : public DeviceBackendException {
    public:

      enum { EX_WRONG_PARAMETER, EX_NOT_OPENED, EX_CANNOT_OPEN_DEVICEBACKEND
      };


      DeviceException(const std::string &_exMessage, unsigned int _exID);
      virtual ~DeviceException() throw();
      friend std::ostream& operator<<(std::ostream &os, const DeviceException& e);
    private:

  };

}//namespace mtca4u

#endif	/* MTCA4U_DEVICE_EXCEPTION_H */

