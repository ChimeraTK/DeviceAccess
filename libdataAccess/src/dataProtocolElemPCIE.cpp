
#include "dataProtocolElemPCIE.h"
#include "exDataProtocol.h"

dataProtocolElemPCIE::dataProtocolElemPCIE(const std::string &_devName, const std::string &_regName, unsigned int _regInternalOffset, unsigned int _regInternalSize, const std::string &devName, mapFile::mapElem& _elem, __DEV__ *_dp, unsigned int _totalRegOffset, unsigned int _totalRegSize)
: dataProtocolElem("PCIE@" + _devName + ":" + _regName),
devName(_devName),
regName(_regName),
regInternalOffset(_regInternalOffset),
regInternalSize(_regInternalSize),
devFileName(devName),
elem(_elem),
totalRegOffset(_totalRegOffset),
totalRegSize(_totalRegSize),
dp(_dp) {
}

dataProtocolElemPCIE::~dataProtocolElemPCIE() {
}

void dataProtocolElemPCIE::readData(rawData &data) {
    if (data.maxDataSize < totalRegSize) {
        throw exDataProtocol("Data buffer too small: " + devName + ":" + regName, exDataProtocol::EX_WRONG_BUFFER_SIZE);
    }
    if (elem.reg_bar == 0xD) {
        dp->readDMA(totalRegOffset, (int32_t*) data.pData, totalRegSize, 0);
    } else {
        dp->readArea(totalRegOffset, (int32_t*) data.pData, totalRegSize, elem.reg_bar);
    }
    data.currentDataSize = totalRegSize;
}

void dataProtocolElemPCIE::writeData(const rawData &data) {
    if (data.maxDataSize != totalRegSize) {
        throw exDataProtocol("Data buffer size differ than register size: " + devName + ":" + regName, exDataProtocol::EX_WRONG_BUFFER_SIZE);
    }
    if (elem.reg_bar == 0xD) {
        dp->writeDMA(totalRegOffset, (int32_t*) data.pData, totalRegSize, 0);
    } else {
        dp->writeArea(totalRegOffset, (int32_t*) data.pData, totalRegSize, elem.reg_bar);
    }
}

void dataProtocolElemPCIE::readMetaData(const std::string& metaDataTag, metaData& mData) {
    if (metaDataTag == "DATA_CHANNEL_INFO") {
        dp->readDeviceInfo(&mData.data);
        mData.data += " [" + devFileName + "]";
        mData.metaDataTag = metaDataTag;
    } else {
        throw exDataProtocol("Unknown metadata tag: \"" + metaDataTag + "\" for data channel: \"" + getAddress() + "\"", exDataProtocol::EX_UNKNOWN_METADATA_TAG);
    }
}

size_t dataProtocolElemPCIE::getDataSize() {
    return totalRegSize;
}

#ifdef __DEBUG_MODE__

std::ostream& dataProtocolElemPCIE::show(std::ostream &os) {
    os << " {[PCIE] D:" << devName << " R:" << regName << " O:" << regInternalOffset << " S:" << regInternalSize << " F:" << devFileName << " " << elem << " [" << totalRegOffset << ", " << totalRegSize << "]}";
    return os;
}
#endif 
