#ifndef EXDEVFAKE_H
#define	EXDEVFAKE_H

#include "exBase.h"
#include <string>

class exDevFake : public exBase {
public:
    enum {
                EX_CANNOT_CREATE_DEV_FILE, 
                EX_DEVICE_FILE_WRITE_DATA_ERROR, 
                EX_DEVICE_FILE_READ_DATA_ERROR, 
                EX_DEVICE_FILE_WRITE_DMA_ERROR, 
                EX_DEVICE_FILE_READ_DMA_ERROR,
                EX_DEVICE_OPENED, 
                EX_DEVICE_CLOSED
    };
public:
    exDevFake(const std::string &_exMessage, unsigned int _exID);
private:

};

#endif	/* EXDEVFAKE_H */

