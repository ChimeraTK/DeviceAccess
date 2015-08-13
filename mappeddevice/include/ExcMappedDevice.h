/* 
 * File:   ExcMappedDevice.h
 * Author: apiotro
 *
 * Created on 11 marzec 2012, 20:42
 */

#ifndef MTCA4U_EXCMAPPEDDEVICE_H
#define	MTCA4U_EXCMAPPEDDEVICE_H

#include "Exception.h"

namespace mtca4u{

class ExcMappedDevice : public ExcBase {
public:
    
  enum { EX_WRONG_PARAMETER, EX_NOT_OPENED, EX_CANNOT_OPEN_DEVBASE
  };
    
    
    ExcMappedDevice(const std::string &_exMessage, unsigned int _exID);
    virtual ~ExcMappedDevice() throw();
    friend std::ostream& operator<<(std::ostream &os, const ExcMappedDevice& e); 
private:

};

}//namespace mtca4u

#endif	/* MTCA4U_EXCMAPPEDDEVICE_H */

