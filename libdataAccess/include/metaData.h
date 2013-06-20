/* 
 * File:   confData.h
 * Author: apiotro
 *
 * Created on 2 kwiecień 2012, 18:44
 */

#ifndef CONFMETADATA_H
#define	CONFMETADATA_H

#include <string>
#include <iostream>

class metaData {
public:
    std::string data;
    std::string metaDataTag;
public:
    metaData();
    virtual ~metaData();
    
#ifdef __DEBUG_MODE__     
    friend std::ostream& operator<<(std::ostream &os, const metaData& mData);
#endif /*__DEBUG_MODE__*/ 
};

#endif	/* CONFMETADATA_H */

