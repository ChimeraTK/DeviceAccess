#include "dataProtocolElem.h"

dataProtocolElem::dataProtocolElem(const std::string &_address)
        : address(_address)
{
}


dataProtocolElem::~dataProtocolElem() {
}

std::string dataProtocolElem::getAddress()
{
    return address;
}

