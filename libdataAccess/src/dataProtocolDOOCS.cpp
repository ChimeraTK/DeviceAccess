#include "dataProtocolDOOCS.h"
#include "exDataProtocol.h"


dataProtocolDOOCS::dataProtocolDOOCS()
 : dataProtocol("DOOCS")
{
}



dataProtocolDOOCS::~dataProtocolDOOCS() {
}


dataProtocolElemDOOCS* dataProtocolDOOCS::createProtocolElem(const std::string& address)
{
    //throw exDataProtocol("Wrong address format in line \"" + protName + "@" + address + "\"", exDataProtocol::EX_WRONG_ADDRESS);
    return new dataProtocolElemDOOCS;
}

void dataProtocolDOOCS::readMetaData(const std::string& logName, const std::string& metaDataTag, metaData& mData)
{
    throw exDataProtocol("Tag \"" + metaDataTag + "\" not supported for \"" + protName + "\" protocol" , exDataProtocol::EX_NOT_SUPPORTED);
}

#ifdef __DEBUG_MODE__ 
std::ostream& dataProtocolDOOCS::show(std::ostream& os)
{
    os << "dataProtocolDOOCS: " << std::endl << "<<EMPTY>>" << std::endl;
    return os;
}
#endif
