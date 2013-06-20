#include "dataProtocolPCIE.h"
#include "exDataProtocol.h"
#include "logicNameMapper.h"
#include <sstream>

dataProtocolPCIE::dataProtocolPCIE(const std::string& dmapFile)
: dataProtocol("PCIE"), dmapFiles(dmapFilesParserSingleton::Instance()) {
    dmapFile::errorList derr;
    mapFile::errorList err;
    dmapFiles.parse_file(dmapFile);
    if (dmapFiles.check(dmapFile::errorList::errorElem::ERROR, mapFile::errorList::errorElem::ERROR, derr, err) == false) {
        std::ostringstream os;
        os << derr << err;
        throw exDataProtocol(os.str(), exDataProtocol::EX_INTERNAL_ERROR);
    }
}

dataProtocolPCIE::~dataProtocolPCIE() {
    std::map<std::string, __DEV__*>::iterator iter;
    for (iter = hwAccess.begin(); iter != hwAccess.end(); iter++) {
        delete (*iter).second;
    }
}

void dataProtocolPCIE::combine(dataProtocol* pDp) {
    throw exDataProtocol("Protocol PCIE already registered", exDataProtocol::EX_PROTOCOL_ALREADY_REGISTERED);
}

dataProtocolElemPCIE* dataProtocolPCIE::createProtocolElem(const std::string& address) {

    std::map<std::string, __DEV__*>::const_iterator citer;
    size_t pos;
    std::string data = address;
    std::string devName;
    std::string regName;
    std::string devFileName;
    unsigned int regInternalOffset = 0;
    unsigned int regInternalSize = 0;
    unsigned int regTotalOffset = 0;
    unsigned int regTotalSize = 0;

    pos = data.find(":");
    if (pos == std::string::npos) {
        throw exDataProtocol(address + " - wrong address format for \"" + protName + "\" protocol", exDataProtocol::EX_WRONG_ADDRESS);
    }
    devName = data.substr(0, pos);
    if (devName.length() == 0) {
        throw exDataProtocol(address + " - wrong address format for \"" + protName + "\" protocol", exDataProtocol::EX_WRONG_ADDRESS);
    }
    data = data.substr(pos + 1);
    pos = data.find(":");
    if (pos == std::string::npos) {
        if (data.length() == 0) {
            throw exDataProtocol(address + " - wrong address format for \"" + protName + "\" protocol", exDataProtocol::EX_WRONG_ADDRESS);
        }
        regName = data;
    } else {
        regName = data.substr(0, pos);
        if (regName.length() == 0) {
            throw exDataProtocol(address + " - wrong address format for \"" + protName + "\" protocol", exDataProtocol::EX_WRONG_ADDRESS);
        }
        data = data.substr(pos + 1);
        pos = data.find(":");
        if (pos != std::string::npos) {
            std::string sub = data.substr(0, pos);
            std::istringstream is(sub);
            is >> std::setbase(0) >> regInternalOffset;
            if (!is) {
                throw exDataProtocol(address + " - wrong address format for \"" + protName + "\" protocol", exDataProtocol::EX_WRONG_ADDRESS);
            }
            data = data.substr(pos + 1);
            if (data.length() != 0) {
                std::istringstream is(data);
                is >> std::setbase(0) >> regInternalSize;
                if (!is) {
                    throw exDataProtocol(address + " - wrong address format for \"" + protName + "\" protocol", exDataProtocol::EX_WRONG_ADDRESS);
                }
            }
        }
    }
    mapFile::mapElem elem;
    try {
        dmapFiles.getRegisterInfo(devName, regName, devFileName, elem);
    } catch (exBase &e) {
        throw exDataProtocol("Error in lmap file: \"" + address + "\": " + e.what(), exDataProtocol::EX_WRONG_ADDRESS);
    }
    if (regInternalOffset + regInternalSize > elem.reg_size || regInternalOffset % 4 || regInternalSize % 4) {
        throw exDataProtocol("Error in lmap file: wrong additional register offset or register size in line \"" + address + "\"", exDataProtocol::EX_WRONG_ADDRESS);
    }

    regTotalOffset = elem.reg_address + regInternalOffset;
    regTotalSize = ((regInternalSize != 0) ? regInternalSize : elem.reg_size);


    __DEV__ *dp;
    citer = hwAccess.find(devFileName);
    if (citer == hwAccess.end()) {
        dp = new __DEV__;
        try {
            dp->openDev(devFileName);
        } catch (...) {
            delete dp;
            throw;
        }
        hwAccess.insert(std::pair<std::string, __DEV__*>(devFileName, dp));
    } else {
        dp = (*citer).second;
    }
    return new dataProtocolElemPCIE(devName, regName, regInternalOffset, regInternalSize, devFileName, elem, dp, regTotalOffset, regTotalSize);
}

void dataProtocolPCIE::readMetaData(const std::string& logName, const std::string& metaDataTag, metaData& mData) {
    size_t pos;
    std::string tagPrefix;
    std::string data;

    pos = metaDataTag.find(":");
    if (pos == std::string::npos) {
        throw exDataProtocol("Tag \"" + metaDataTag + "\" not supported for \"" + protName + "\" protocol", exDataProtocol::EX_NOT_SUPPORTED);
    }
    tagPrefix = metaDataTag.substr(0, pos);
    if (tagPrefix.length() == 0) {
        throw exDataProtocol("Tag \"" + metaDataTag + "\" not supported for \"" + protName + "\" protocol", exDataProtocol::EX_NOT_SUPPORTED);
    }
    data = metaDataTag.substr(pos + 1);   
    if (tagPrefix == "MAP") {
        std::string devName;
        std::string address = lnm->get(logName)->getAddress();                
        mData.metaDataTag = metaDataTag;
        pos = address.find(":");
        if (pos == std::string::npos) {
            throw exDataProtocol(address + " - wrong address format for \"" + protName + "\" protocol", exDataProtocol::EX_WRONG_ADDRESS);
        }
        devName = address.substr(0, pos);        
        if (devName.length() == 0) {
            throw exDataProtocol(address + " - wrong address format for \"" + protName + "\" protocol", exDataProtocol::EX_WRONG_ADDRESS);
        }
        dmapFiles.getMapFile(devName)->getMetaData(data, mData.data);
    } else {
        throw exDataProtocol("Tag \"" + metaDataTag + "\" not supported for \"" + protName + "\" protocol", exDataProtocol::EX_NOT_SUPPORTED);
    }
}

#ifdef __DEBUG_MODE__ 

std::ostream& dataProtocolPCIE::show(std::ostream& os) {
    std::map<std::string, __DEV__*>::iterator iter;
    os << "dataProtocolPCIE: " << std::endl;
    os << dmapFiles;
    for (iter = hwAccess.begin(); iter != hwAccess.end(); ++iter) {
        os << "Device File: " << (*iter).first << std::endl;
    }
    return os;
}
#endif
