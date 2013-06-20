#ifndef _DEVMAP_H
#define	_DEVMAP_H

/**
 *      @file           devMap.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Template that connect functionality of libdev and libmap libraries. 
 *                      This file support only map file parsing. 
 *                  
 */

#include "libmap.h"
#include "libdev.h"
#include "refCountPointer.h"
#include "exdevMap.h"
 

/**
 *      @class  devMap
 *      @brief  Class allows to read/write registers from device
 * 
 *      Allows to read/write registers from device by passing the name of 
 *      the register instead of offset from the beginning of address space.
 *      Type of the object used to control access to device must be passed
 *      as a template parameter and must be an type defined in libdev class.
 *      
 */
template<typename T>
class devMap {
    
public:
    typedef ref_count_pointer<T>      ptrdev;    
private:
    
    ptrdev              pdev;
    std::string         mapFileName;
    ptrmapFile          mapFile;
    
public:            
    
    class regObject
    {
            std::string                 regName;
            mapFile::mapElem            me;
            typename devMap::ptrdev     pdev;
        private:
            void checkRegister(const mapFile::mapElem &me, size_t dataSize, uint32_t addRegOffset, uint32_t &retDataSize, uint32_t &retRegOff);
        public:
            regObject(const std::string &_regName, const mapFile::mapElem &_me, typename devMap::ptrdev _pdev);
            void readReg(int32_t* data, size_t dataSize = 0, uint32_t addRegOffset = 0);
            void writeReg(int32_t* data, size_t dataSize = 0, uint32_t addRegOffset = 0);
            void readDMA(int32_t* data, size_t dataSize = 0, uint32_t addRegOffset = 0);
            void writeDMA(int32_t* data, size_t dataSize = 0, uint32_t addRegOffset = 0);
    };
    
    devMap();   
    virtual void openDev(const std::string &_devFileName, const std::string& _mapFileName, int _perm = O_RDWR, devConfigBase* _pConfig = NULL);
    virtual void closeDev();    
    virtual void readReg(uint32_t regOffset, int32_t* data, uint8_t bar);
    virtual void writeReg(uint32_t regOffset, int32_t data, uint8_t bar);    
    virtual void readArea(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar);
    virtual void writeArea(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar);    
    virtual void readDMA(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar);
    virtual void writeDMA(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar);     
    virtual void readDeviceInfo(std::string* devInfo);
    
    
    virtual void readReg(const std::string &regName, int32_t* data, size_t dataSize = 0, uint32_t addRegOffset = 0);
    virtual void writeReg(const std::string &regName, int32_t* data, size_t dataSize = 0, uint32_t addRegOffset = 0);
    virtual void readDMA(const std::string &regName, int32_t* data, size_t dataSize = 0, uint32_t addRegOffset = 0);
    virtual void writeDMA(const std::string &regName, int32_t* data, size_t dataSize = 0, uint32_t addRegOffset = 0);
    
    
    regObject  getRegObject(const std::string &regName);
    
    virtual ~devMap();

private:
    void checkRegister(const std::string &regName, size_t dataSize, uint32_t addRegOffset, uint32_t &retDataSize, uint32_t &retRegOff, uint8_t &retRegBar);
};

template<typename T>
devMap<T>::devMap()
{

}

template<typename T>
devMap<T>::~devMap() {
    if (pdev)
        pdev->closeDev();
}

template<typename T>
typename devMap<T>::regObject  devMap<T>::getRegObject(const std::string &regName)
{
    mapFile::mapElem    me;
    mapFile->getRegisterInfo(regName, me);
    return devMap::regObject(regName, me, pdev);
}

template<typename T>
void devMap<T>::checkRegister(const std::string &regName, size_t dataSize, uint32_t addRegOffset, uint32_t &retDataSize, uint32_t &retRegOff, uint8_t &retRegBar)
{
    mapFile::mapElem    me;
    mapFile->getRegisterInfo(regName, me);
    if (addRegOffset % 4){
        throw exdevMap("Register offset must be dividable by 4", exdevMap::EX_WRONG_PARAMETER);
    }
    if (dataSize){
        if (dataSize % 4){
            throw exdevMap("Data size must be dividable by 4", exdevMap::EX_WRONG_PARAMETER);
        }
        if (dataSize > me.reg_size - addRegOffset){
            throw exdevMap("Data size exceed register size", exdevMap::EX_WRONG_PARAMETER);
        }        
        retDataSize = dataSize;
    } else {
        retDataSize = me.reg_size;
    }
    retRegBar = me.reg_bar;
    retRegOff = me.reg_address + addRegOffset;
}

template<typename T>
void devMap<T>::readReg(const std::string &regName, int32_t* data, size_t dataSize, uint32_t addRegOffset)
{
    uint32_t retDataSize;
    uint32_t retRegOff;
    uint8_t  retRegBar;
    
    checkRegister(regName, dataSize, addRegOffset, retDataSize, retRegOff, retRegBar);
    readArea(retRegOff, data, retDataSize, retRegBar);
}

template<typename T>
void devMap<T>::writeReg(const std::string &regName, int32_t* data, size_t dataSize, uint32_t addRegOffset)
{
    uint32_t retDataSize;
    uint32_t retRegOff;
    uint8_t  retRegBar;
    
    checkRegister(regName, dataSize, addRegOffset, retDataSize, retRegOff, retRegBar);
    writeArea(retRegOff, data, retDataSize, retRegBar);
}

template<typename T>
void devMap<T>::readDMA(const std::string &regName, int32_t* data, size_t dataSize, uint32_t addRegOffset)
{
    uint32_t retDataSize;
    uint32_t retRegOff;
    uint8_t  retRegBar;
    
    checkRegister(regName, dataSize, addRegOffset, retDataSize, retRegOff, retRegBar);
    if (retRegBar != 0xD){
        throw exdevMap("Cannot read data from register \"" + regName + "\" through DMA", exdevMap::EX_WRONG_PARAMETER);
    }
    readDMA(retRegOff, data, retDataSize, retRegBar);
}

template<typename T>
void devMap<T>::writeDMA(const std::string &regName, int32_t* data, size_t dataSize, uint32_t addRegOffset)
{
    uint32_t retDataSize;
    uint32_t retRegOff;
    uint8_t  retRegBar;
    checkRegister(regName, dataSize, addRegOffset, retDataSize, retRegOff, retRegBar);
    if (retRegBar != 0xD){
        throw exdevMap("Cannot write data from register \"" + regName + "\" through DMA", exdevMap::EX_WRONG_PARAMETER);
    }    
    writeDMA(retRegOff, data, retDataSize, retRegBar);
}

/**
 *      @brief  Function allows to open device specified by the name 
 * 
 *      Function throws the same exceptions like openDev from class type
 *      passed as a template parameter.  
 * 
 *      @param  _devFileName - name of the device
 *      @param  _mapFileName -  name of the map file string information about 
 *                              registers available in device memory space.
 *      @param  _perm        -  permitions for the device file in form accepted 
 *                              by standard open function [default: O_RDWR]  
 *      @param  _pConfig     -  additional configuration used to prepare device.
 *                              Structure of this parameter depends on type of 
 *                              the device [default: NULL]     
 */
template<typename T>
void devMap<T>::openDev(const std::string &_devFileName, const std::string& _mapFileName, int _perm, devConfigBase* _pConfig)
{
    mapFileParser fileParser;
    mapFileName = _mapFileName;    
    mapFile     = fileParser.parse(mapFileName);
    pdev        = new T;
    pdev->openDev(_devFileName, _perm, _pConfig);
}

/**
 *      @brief  Function allows to close device 
 * 
 *      Function throws the same exceptions like closeDev from class type
 *      passed as a template parameter.      
 */
template<typename T>
void devMap<T>::closeDev()
{
    pdev->closeDev();
}
    
/**
 *      @brief  Function allows to read data from one register located in 
 *              device address space
 * 
 *      This is wrapper to standard readReg function defined in libdev library. 
 *      Allows to read one register located in device address space. Size of register
 *      depends on type of accessed device e.x. for PCIe device it is equal to 
 *      32bit. Function throws the same exceptions like readReg from class type.
 * 
 * 
 *      @param  regOffset - offset of the register in device address space
 *      @param  data - pointer to area to store data 
 *      @param  bar  - number of PCIe bar 
 */
template<typename T>
void devMap<T>::readReg(uint32_t regOffset, int32_t* data, uint8_t bar)
{
    pdev->readReg(regOffset, data, bar);
}

/**
 *      @brief  Function allows to write data to one register located in 
 *              device address space
 * 
 *      This is wrapper to standard writeReg function defined in libdev library. 
 *      Allows to write one register located in device address space. Size of register
 *      depends on type of accessed device e.x. for PCIe device it is equal to 
 *      32bit. Function throws the same exceptions like writeReg from class type.
 * 
 *      @param  regOffset - offset of the register in device address space
 *      @param  data - pointer to data to write 
 *      @param  bar  - number of PCIe bar 
 */
template<typename T>
void devMap<T>::writeReg(uint32_t regOffset, int32_t data, uint8_t bar)
{
    pdev->writeReg(regOffset, data, bar);
}

/**
 *      @brief  Function allows to read data from several registers located in 
 *              device address space
 * 
 *      This is wrapper to standard readArea function defined in libdev library. 
 *      Allows to read several registers located in device address space. 
 *      Function throws the same exceptions like readArea from class type.
 * 
 * 
 *      @param  regOffset - offset of the register in device address space
 *      @param  data - pointer to area to store data 
 *      @param  size - number of bytes to read from device
 *      @param  bar  - number of PCIe bar 
 */
template<typename T>
void devMap<T>::readArea(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar)
{
    pdev->readArea(regOffset, data, size, bar);
}

template<typename T>
void devMap<T>::writeArea(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar)
{
    pdev->writeArea(regOffset, data, size, bar);
}
    
template<typename T>
void devMap<T>::readDMA(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar)
{
    pdev->readDMA(regOffset, data, size, bar);
}

template<typename T>
void devMap<T>::writeDMA(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar)
{
    pdev->writeDMA(regOffset, data, size, bar);
}
    
template<typename T>
void devMap<T>::readDeviceInfo(std::string* devInfo)
{
    pdev->readDeviceInfo(devInfo);
}


template<typename T>
devMap<T>::regObject::regObject(const std::string &_regName, const mapFile::mapElem &_me, ptrdev _pdev)
: regName(_regName), me(_me), pdev(_pdev)
{
        
    
}

template<typename T>
void devMap<T>::regObject::checkRegister(const mapFile::mapElem &me, size_t dataSize, uint32_t addRegOffset, uint32_t &retDataSize, uint32_t &retRegOff)
{    
    if (addRegOffset % 4){
        throw exdevMap("Register offset must be dividable by 4", exdevMap::EX_WRONG_PARAMETER);
    }
    if (dataSize){
        if (dataSize % 4){
            throw exdevMap("Data size must be dividable by 4", exdevMap::EX_WRONG_PARAMETER);
        }
        if (dataSize > me.reg_size - addRegOffset){
            throw exdevMap("Data size exceed register size", exdevMap::EX_WRONG_PARAMETER);
        }        
        retDataSize = dataSize;
    } else {
        retDataSize = me.reg_size;
    }
    retRegOff = me.reg_address + addRegOffset;
}


template<typename T>
void devMap<T>::regObject::readReg(int32_t* data, size_t dataSize, uint32_t addRegOffset)
{
    uint32_t retDataSize;
    uint32_t retRegOff;    
    checkRegister(me, dataSize, addRegOffset, retDataSize, retRegOff);
    pdev->readArea(retRegOff, data, retDataSize, me.reg_bar);
}

template<typename T>
void devMap<T>::regObject::writeReg(int32_t* data, size_t dataSize, uint32_t addRegOffset)
{
    uint32_t retDataSize;
    uint32_t retRegOff;    
    checkRegister(me, dataSize, addRegOffset, retDataSize, retRegOff);
    pdev->writeArea(retRegOff, data, retDataSize, me.reg_bar);
}

template<typename T>
void devMap<T>::regObject::readDMA(int32_t* data, size_t dataSize, uint32_t addRegOffset)
{
    uint32_t retDataSize;
    uint32_t retRegOff;    
    checkRegister(me, dataSize, addRegOffset, retDataSize, retRegOff);
    if (me.reg_bar != 0xD){
        throw exdevMap("Cannot read data from register \"" + regName + "\" through DMA", exdevMap::EX_WRONG_PARAMETER);
    }
    pdev->readDMA(retRegOff, data, retDataSize, me.reg_bar);
}

template<typename T>
void devMap<T>::regObject::writeDMA(int32_t* data, size_t dataSize, uint32_t addRegOffset)
{
    uint32_t retDataSize;
    uint32_t retRegOff;    
    checkRegister(me, dataSize, addRegOffset, retDataSize, retRegOff);
    if (me.reg_bar != 0xD){
        throw exdevMap("Cannot read data from register \"" + regName + "\" through DMA", exdevMap::EX_WRONG_PARAMETER);
    }
    pdev->writeDMA(retRegOff, data, retDataSize, me.reg_bar);
}



#endif	/* _DEVMAP_H */

