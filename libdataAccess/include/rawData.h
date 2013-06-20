
#ifndef RAWDATA_H
#define	RAWDATA_H

#include <stdint.h>
#include <ostream>

class rawData {
private:
    bool internalAllocation;
public:
    union {
        void        *vData;
        uint8_t     *pData;
        float       *fData;
        int32_t     *iData;
        uint32_t    *uData;
    };
    size_t    	maxDataSize;
    size_t    	currentDataSize;
public:
    rawData();
    rawData(size_t _maxDataSize);
    rawData(void* _pData, size_t _maxDataSize);
    void init(size_t _maxDataSize);
    virtual ~rawData();
    bool wasInternalAlloc();
#ifdef __DEBUG_MODE__   
    void show(std::ostream &os, uint32_t size_words, uint32_t offset_words);
    friend std::ostream& operator<<(std::ostream &os, const rawData& rData);
#endif /*__DEBUG_MODE__ */
};

#endif	/* RAWDATA_H */

