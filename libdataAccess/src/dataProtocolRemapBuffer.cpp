/* 
 * File:   dataProtocolBuffer.cpp
 * Author: apiotro
 * 
 * Created on 28 styczeń 2012, 21:01
 */

#include "dataProtocolRemapBuffer.h"
#include "exDataProtocol.h"
#include "dbg_print.h"
#include "logicNameMapper.h"
#include <iostream>
#include <sstream>

dataProtocolRemapBuffer::dataProtocolRemapBuffer()
: dataProtocol("REMAPBUFFER"), dmapFiles(dmapFilesParserSingleton::Instance()) {
}

dataProtocolRemapBuffer::~dataProtocolRemapBuffer() {
}

dataProtocolElemRemapBuffer* dataProtocolRemapBuffer::createProtocolElem(const std::string& address) {

    std::string data = address;
    size_t pos;
    std::string sval;
    std::string buffName;
    std::string devName;
    std::string regName;
    std::string devFileName;
    std::string addressMethod;
    int mappingOffset;
    unsigned int regInternalOffset = 0;
    unsigned int regInternalSize = 0;
    unsigned int regTotalOffset = 0;
    unsigned int regTotalSize = 0;



    pos = data.find(":");
    if (pos == std::string::npos) {
        throw exDataProtocol(address + " - wrong address format for \"" + protName + "\" protocol", exDataProtocol::EX_WRONG_ADDRESS);
    }
    buffName = data.substr(0, pos);
    if (buffName.length() == 0) {
        throw exDataProtocol(address + " - wrong address format for \"" + protName + "\" protocol", exDataProtocol::EX_WRONG_ADDRESS);
    }
    data = data.substr(pos + 1);
    pos = data.find(":");
    if (pos == std::string::npos) {
        throw exDataProtocol(address + " - wrong address format for \"" + protName + "\" protocol", exDataProtocol::EX_WRONG_ADDRESS);
    }
    addressMethod = data.substr(0, pos);
    data = data.substr(pos + 1);
    if (addressMethod == "MAP") {
        pos = data.find(":");
        if (pos == std::string::npos) {
            throw exDataProtocol(address + " - wrong address format for \"" + protName + "\" protocol (wrong mapping offset)", exDataProtocol::EX_WRONG_ADDRESS);
        }
        sval = data.substr(0, pos);
        if (sval.length() == 0) {
            throw exDataProtocol(address + " - wrong address format for \"" + protName + "\" protocol (wrong mapping offset)", exDataProtocol::EX_WRONG_ADDRESS);
        }
        std::istringstream is(sval);
        is >> std::setbase(0) >> mappingOffset;
        if (!is) {
            throw exDataProtocol(address + " - wrong address format for \"" + protName + "\" protocol (wrong mapping offset)", exDataProtocol::EX_WRONG_ADDRESS);
        }
        data = data.substr(pos + 1);
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

        regTotalOffset = elem.reg_address + regInternalOffset + mappingOffset;
        regTotalSize = ((regInternalSize != 0) ? regInternalSize : elem.reg_size);

    } else if (addressMethod == "DIRECT") {
        pos = data.find(":");
        if (pos == std::string::npos) {
            throw exDataProtocol(address + " - wrong address format for \"" + protName + "\" protocol", exDataProtocol::EX_WRONG_ADDRESS);
        }
        sval = data.substr(0, pos);
        if (sval.length() == 0) {
            throw exDataProtocol(address + " - wrong address format for \"" + protName + "\" protocol", exDataProtocol::EX_WRONG_ADDRESS);
        }
        std::istringstream is(sval);
        is >> std::setbase(0) >> regTotalOffset;
        if (!is) {
            throw exDataProtocol(address + " - wrong address format for \"" + protName + "\" protocol", exDataProtocol::EX_WRONG_ADDRESS);
        }
        data = data.substr(pos + 1);
        if (data.length() != 0) {
            std::istringstream is(data);
            is >> std::setbase(0) >> regTotalSize;
            if (!is) {
                throw exDataProtocol(address + " - wrong address format for \"" + protName + "\" protocol", exDataProtocol::EX_WRONG_ADDRESS);
            }
        }
        devName = "NA";
        regName = "NA";

    } else {
        throw exDataProtocol(address + " - unknown address access method", exDataProtocol::EX_WRONG_ADDRESS);
    }

    std::map<std::string, rawData*>::iterator citer;
    citer = buffers.find(buffName);
    if (citer == buffers.end()) {
        return NULL;
    }
    return new dataProtocolElemRemapBuffer(regTotalOffset, regTotalSize, (*citer).second, buffName + ":" + devName + ":" + regName);
}

void dataProtocolRemapBuffer::addBuffer(const std::string& bufName, rawData* buff) {
    std::map<std::string, rawData*>::iterator iter;
    iter = buffers.find(bufName);
    if (iter == buffers.end())
        buffers.insert(std::pair<std::string, rawData*>(bufName, buff));
    else
        throw exDataProtocol("Buffer \"" + bufName + "\" already registered", exDataProtocol::EX_BUFFER_ALREADY_REGISTERED);
}

void dataProtocolRemapBuffer::combine(dataProtocol* pD) {
    dataProtocolRemapBuffer* pDp = dynamic_cast<dataProtocolRemapBuffer*> (pD);
    std::map<std::string, rawData*>::iterator iter;
    std::map<std::string, rawData*>::iterator siter;
    for (iter = pDp->buffers.begin(); iter != pDp->buffers.end(); ++iter) {
        siter = buffers.find((*iter).first);
        if (siter != buffers.end()) {
            if ((*siter).second == NULL) {
                (*siter).second = (*iter).second;
            } else {
                throw exDataProtocol("Buffer \"" + (*iter).first + "\" already registered", exDataProtocol::EX_BUFFER_ALREADY_REGISTERED);
            }
        } else {
            buffers.insert(*iter);
        }
    }

    logicNameMapper::iterator it;
    std::string protocolName;
    std::string address;

    logicNameMapper* lm = const_cast<logicNameMapper*> (lnm);

    for (it = lm->begin(); it != lm->end(); ++it) {
        protocolName = (*it)->getProtocolName();
        if (protocolName != getProtocolName()) {
            continue;
        }
        address = (*it)->getAddress();
        if ((*it)->getDataProtocolElem() == 0) {
            (*it)->setDataProtocolElem(createProtocolElem(address));
        }
    }
}

void dataProtocolRemapBuffer::readMetaData(const std::string& logName, const std::string& metaDataTag, metaData& mData)
{
    throw exDataProtocol("Tag \"" + metaDataTag + "\" not supported for \"" + protName + "\" protocol" , exDataProtocol::EX_NOT_SUPPORTED);
}

#ifdef __DEBUG_MODE__
std::ostream& dataProtocolRemapBuffer::show(std::ostream &os) {

    std::map<std::string, rawData*>::iterator iter;
    os << "dataProtocolDirectBuffer: " << std::endl;
    for (iter = buffers.begin(); iter != buffers.end(); ++iter) {
        os << "Buffers: " << (*iter).first << std::endl;
    }
    return os;
}
#endif /*__DEBUG_MODE__*/