/* 
 * File:   ExcDevice.h
 * Author: apiotro
 *
 * Created on 11 marzec 2012, 20:42
 */

#ifndef MTCA4U_EXCMAPPEDDEVICE_H
#define	MTCA4U_EXCMAPPEDDEVICE_H

#include "DeviceBackend.h"
namespace mtca4u{

class DeviceException : public DeviceBackendException {
public:

	enum { EX_WRONG_PARAMETER, EX_NOT_OPENED, EX_CANNOT_OPEN_DEVBASE
	};


	DeviceException(const std::string &_exMessage, unsigned int _exID);
	virtual ~DeviceException() throw();
	friend std::ostream& operator<<(std::ostream &os, const DeviceException& e);
private:

};

}//namespace mtca4u

#endif	/* MTCA4U_EXCMAPPEDDEVICE_H */

