/**
 *      @file           dataProtocol.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Provides base class for protocol handling object                
 */
#ifndef __DATA_PROTOCOL_H__
#define __DATA_PROTOCOL_H__

#include <string>
#include "dataProtocolElem.h"
#include "metaData.h"

class logicNameMapper;

/**
 *      @brief Base class for communication protocols handled by logical name mapper
 *
 */
class dataProtocol {
protected:
    const std::string protName; /**< name of protocol*/
    const logicNameMapper* lnm; /**< pointer to object that stores information about all currently registered logical names*/

public:
    /**
     *  @brief Class constructor
     *  @param _protName - name of the protocol - must be the same like protcol name from lmap file
     */
    dataProtocol(const std::string& _protName);
    virtual ~dataProtocol();
    std::string getProtocolName() const;
    virtual dataProtocolElem* createProtocolElem(const std::string& address) = 0;
    virtual void readMetaData(const std::string& logName, const std::string& metaDataTag, metaData& mData) = 0;
    virtual void combine(dataProtocol* pDp);
    void setLogicNameMapper(const logicNameMapper* _lnm);

#ifdef __DEBUG_MODE__     
    virtual std::ostream& show(std::ostream &os) = 0;
#endif

};

#endif //__DATA_PROTOCOL_H__