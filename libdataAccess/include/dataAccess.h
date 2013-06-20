/**
 *      @file           dataAccess.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Provides interface for logical names mapping mechanism                 
 */
#ifndef __DATA_ACCESS_H__
#define __DATA_ACCESS_H__

#include "dataProtocol.h"
#include "dataProtocolManager.h"
#include "logicNameMapper.h"
#include "rawData.h"
#include "singleton.h"
#include "libmap.h"
#include "metaData.h"

/**
 *      @brief  Provides interface to logical name mapping mechanism. 
 *      
 *      Provides interface for access to data through logical data mapping. Stores information about
 *      available data channels, registered logical names and provides functions to read/write data. Allows to dynamically extend supported
 *      data channelsby dynamic registration of new communication protocols. Support reading of metadata 
 *      specific to selected communication channel.   
 *
 */
class dataAccess {
private:
    dataProtocolManager dpm; /**< stores information about all currently registered protocols*/
    logicNameMapper lnm; /**< stores information about all currently registered logical names*/

public:

    /**
     * @brief Defines metadata level
     */
    enum metaDataLevel {
        PROTOCOL_LEVEL /**< metadata specific for data access protocol - e.x. meta data from map or dmap files for PCIe protocol*/
        , DATA_CHANNEL_LEVEL /**< metadata specific for data channel - e.x. name of device driver associated with specified logical name*/
    };
public:
    /**
     * Default class constructor - empty
     */
    dataAccess();
    /**
     * Class destructor
     */
    virtual ~dataAccess();
    /**
     * @brief Parse configuration file and initialize all logical names 
     * 
     * Function parses specified as a parameter lmap file, and creates all data protocol objects 
     * responsible for access to logical names. 
     * 
     * @param lmapFileName - name of lmap configuration file
     */
    void init(const std::string& lmapFileName);
    /**
     * @brief Register support for new protocol
     * 
     * @param pDp - pointer to object handling protocol operations
     */
    void addProtocol(dataProtocol* pDp);
    /**
     * @brief Read data from specified logical name
     * 
     * @param logName - logical name of data source
     * @param data - variable to store data
     */
    void readData(const std::string& logName, rawData& data);
    /**
     * @brief Write data to specified logical name
     * 
     * @param logName - logical name of data target
     * @param data - variable that store data to write
     */
    void writeData(const std::string& logName, const rawData& data);
    /**
     * @brief Read metadata associated with protocol or data channel
     * 
     * @param level - metadata level - PROTOCOL_LEVEL or DATA_CHANNEL_LEVEL
     * @param logName - logical name of data source
     * @param metaDataTag - name of metadata 
     * @param mData - variable to store metadata
     */
    void readMetaData(metaDataLevel level, const std::string& logName, const std::string& metaDataTag, metaData& mData);
    /**
     * @brief Return device object associated with specified logical name
     * 
     * @param logName - logical name of data source/data target
     * @return device object associated with specified logical name
     */
    dataProtocolElem* getDeviceObject(const std::string& logName);
#ifdef __DEBUG_MODE__    
    friend std::ostream& operator<<(std::ostream& os, const dataAccess& da);
#endif   
};

/**
 * @typedef SingletonHolder<dataAccess, CreateByNew, LifetimeStandard, SingleThread> dataAccessSingleton
 *      
 * Introduce specialisation of SingletonHolder template for dataAccess
 */
typedef SingletonHolder<dataAccess, CreateByNew, LifetimeStandard, SingleThread> dataAccessSingleton;

#endif //__DATA_ACCESS_H__

