/* 
 * File:   exdevMap.h
 * Author: apiotro
 *
 * Created on 11 marzec 2012, 20:42
 */

#ifndef EXDEVMAP_H
#define	EXDEVMAP_H

#include "exBase.h"

class exdevMap : public exBase {
public:
    
  enum { EX_WRONG_PARAMETER, EX_NOT_OPENED
  };
    
    
    exdevMap(const std::string &_exMessage, unsigned int _exID);
    virtual ~exdevMap() throw();
    friend std::ostream& operator<<(std::ostream &os, const exdevMap& e); 
private:

};

#endif	/* EXDEVMAP_H */

