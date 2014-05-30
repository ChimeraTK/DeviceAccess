#ifndef _MTCA4U_DEVBASEIMPL_H__
#define _MTCA4U_DEVBASEIMPL_H__

#include "devBase.h"

namespace mtca4u{

/** devBaseImpl implements the "opened" functionality which before was in devBase.
 *  It is to be a base class for all the other implementations. Like this debBase
 *  becomes purely virtual, i.e. a read interface.
 */
class devBaseImpl: public devBase
{   
protected:
    bool         opened;
public:
    devBaseImpl() : opened(false){}
    virtual ~devBaseImpl(){}
             
    /** Return whether a device has been opened or not.
     *  This is the only function implemented here to avoid code duplication.
     *  All other functions stay purely virtual.
     */
     virtual bool isOpen(){ return opened; }
};

}//namespace mtca4u

#endif /*_MTCA4U_LIBDEV_H__*/
