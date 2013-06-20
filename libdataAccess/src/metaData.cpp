/* 
 * File:   confData.cpp
 * Author: apiotro
 * 
 * Created on 2 kwiecień 2012, 18:44
 */

#include "metaData.h"

metaData::metaData() {
}

metaData::~metaData() {
}

#ifdef __DEBUG_MODE__ 
std::ostream& operator<<(std::ostream &os, const metaData& mData)
{
    os << "TAG: " << mData.metaDataTag << ":" << mData.data;
    return os;
}
#endif /*__DEBUG_MODE__*/
