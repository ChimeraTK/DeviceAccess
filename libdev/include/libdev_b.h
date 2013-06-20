#ifndef __LIBDEV_H__
#define __LIBDEV_H__


#include <string>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


#define STATUS_OK       0
#define STATUS_CLOSED   1
#define STATUS_FAILURE  3
#define OPEN_FAILURE    4
#define READ_FAILURE    5
#define WRITE_FAILURE   6
#define NOT_SUPPORTED   10

#define SET_LAST_ERROR(s, m)                                                    \
						do {				\
							last_error_str = m;     \
							last_error = s;		\
                                                        return last_error;      \
						} while (0);

#define SET_STATUS(s, m)                                                        \
						do{                             \
                                                        last_error = s;         \
                                                        last_error_str = m;     \
                                                        status = s;		\
                                                        return status;          \
						} while (0);

class devConfig_Base
{
public:
    virtual ~devConfig_Base(){}
};


class devAccess
{
protected:
    uint16_t            status;
    uint16_t            last_error;
    std::string         last_error_str;
    char                err_buff[255];
    
public:
    devAccess() : status(STATUS_CLOSED), last_error(STATUS_CLOSED), last_error_str("Device closed") {};
    ~devAccess() {};
             
    virtual uint16_t openDev(const std::string &dev_name, int perm = O_RDWR, devConfig_Base* pconfig = NULL) = 0;
    virtual uint16_t closeDev() = 0;
    
    virtual uint16_t readReg(uint32_t reg_offset, int32_t* data, uint8_t bar) {SET_LAST_ERROR(NOT_SUPPORTED, "Operation not supported yet");}
    virtual uint16_t writeReg(uint32_t reg_offset, int32_t data, uint8_t bar) {SET_LAST_ERROR(NOT_SUPPORTED, "Operation not supported yet");}
    
    virtual uint16_t readArea(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar) {SET_LAST_ERROR(NOT_SUPPORTED, "Operation not supported yet");}
    virtual uint16_t writeArea(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar) {SET_LAST_ERROR(NOT_SUPPORTED, "Operation not supported yet");}
    
    virtual uint16_t readDMA(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar) {SET_LAST_ERROR(NOT_SUPPORTED, "Operation not supported yet");}
    virtual uint16_t writeDMA(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar) {SET_LAST_ERROR(NOT_SUPPORTED, "Operation not supported yet");}
    
    virtual uint16_t writeData(int32_t* data, size_t &size) {SET_LAST_ERROR(NOT_SUPPORTED, "Operation not supported yet");}
    virtual uint16_t readData(int32_t* data, size_t &size)  {SET_LAST_ERROR(NOT_SUPPORTED, "Operation not supported yet");}
    
    std::string getLastErrorString()  {return last_error_str;}
    uint16_t    getLastError()        {return last_error;} 
};

#endif /*__LIBDEV_H__*/