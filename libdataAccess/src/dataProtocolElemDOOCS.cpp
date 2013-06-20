/* 
 * File:   dataProtocolElemDOOCS.cpp
 * Author: apiotro
 * 
 * Created on 28 grudzień 2011, 15:31
 */

#include "dataProtocolElemDOOCS.h"
#include "exDataProtocol.h"

dataProtocolElemDOOCS::dataProtocolElemDOOCS()
        : dataProtocolElem("")
{
}


dataProtocolElemDOOCS::~dataProtocolElemDOOCS() {
}

void dataProtocolElemDOOCS::readData(rawData& data)
{
    
}

void dataProtocolElemDOOCS::writeData(const rawData& data)
{
    
}
void dataProtocolElemDOOCS::readMetaData(const std::string& metaDataTag, metaData& mData)
{
    throw exDataProtocol("Unknown metadata tag: \"" + metaDataTag + "\" for data channel: \"" + getAddress() + "\"", exDataProtocol::EX_UNKNOWN_METADATA_TAG);
}

size_t dataProtocolElemDOOCS::getDataSize()
{
	return 0;
}

#ifdef __DEBUG_MODE__
std::ostream& dataProtocolElemDOOCS::show(std::ostream &os)
{
    os << " {[DOOCS]";
    return os;
}
#endif 
