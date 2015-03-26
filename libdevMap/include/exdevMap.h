/* 
 * File:   exdevMap.h
 * Author: apiotro
 *
 * Created on 11 marzec 2012, 20:42
 */

#ifndef MTCA4U_EXDEVMAP_H
#define	MTCA4U_EXDEVMAP_H

#include "exBase.h"

namespace mtca4u{

class exdevMap : public exBase {
public:
    
  enum { EX_WRONG_PARAMETER, EX_NOT_OPENED, EX_CANNOT_OPEN_DEVBASE
  };
    
    
    exdevMap(const std::string &_exMessage, unsigned int _exID);
    virtual ~exdevMap() throw();
    friend std::ostream& operator<<(std::ostream &os, const exdevMap& e); 
private:

};

}//namespace mtca4u

#endif	/* MTCA4U_EXDEVMAP_H */

