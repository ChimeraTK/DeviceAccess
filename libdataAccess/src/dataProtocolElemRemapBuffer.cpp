/* 
 * File:   dataProtocolElemBuffer.cpp
 * Author: apiotro
 * 
 * Created on 28 styczeń 2012, 21:02
 */

#include "dataProtocolElemRemapBuffer.h"
#include "exDataProtocol.h"
#include <cstring>

dataProtocolElemRemapBuffer::dataProtocolElemRemapBuffer(unsigned int _regTotalOffset, unsigned int _regTotalSize, rawData* _buff, const std::string &address)
: dataProtocolElem(address), regTotalOffset(_regTotalOffset), regTotalSize(_regTotalSize), buff(_buff) {
}

dataProtocolElemRemapBuffer::~dataProtocolElemRemapBuffer() {
}

void dataProtocolElemRemapBuffer::readData(rawData& data) {
    data.pData = buff->pData + regTotalOffset;
    data.currentDataSize = regTotalSize;
    data.maxDataSize = 0;
}

void dataProtocolElemRemapBuffer::writeData(const rawData& data) {
    throw exDataProtocol("Write operation not supported in BUFFER protocol", exDataProtocol::EX_NOT_SUPPORTED);
}

void dataProtocolElemRemapBuffer::readMetaData(const std::string& metaDataTag, metaData& mData) {
    if (metaDataTag == "DATA_CHANNEL_INFO") {
        mData.data = getAddress();
    } else {
        throw exDataProtocol("Unknown metadata tag: \"" + metaDataTag + "\" for data channel: \"" + getAddress() + "\"", exDataProtocol::EX_UNKNOWN_METADATA_TAG);
    }
}

size_t dataProtocolElemRemapBuffer::getDataSize() {
    return regTotalSize;
}

#ifdef __DEBUG_MODE__     

std::ostream& dataProtocolElemRemapBuffer::show(std::ostream &os) {
    os << " {[BUFFER] " << getAddress() << " [" << regTotalOffset << ", " << regTotalSize << "]" << "}";
    return os;
}
#endif /*__DEBUG_MODE__*/
