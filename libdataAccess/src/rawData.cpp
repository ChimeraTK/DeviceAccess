#include "rawData.h"
#include <iomanip>

rawData::rawData()
 : internalAllocation(false), pData(NULL), maxDataSize(0), currentDataSize(0)
{
}

rawData::rawData(size_t _maxDataSize)
: internalAllocation(true), maxDataSize(_maxDataSize), currentDataSize(0)
{
    pData = new uint8_t[maxDataSize];
}

rawData::rawData(void* _pData, size_t _maxDataSize)
 : internalAllocation(false), vData(_pData), maxDataSize(_maxDataSize), currentDataSize(_maxDataSize)
{}

void rawData::init(size_t _maxDataSize)
{
    if (internalAllocation)
        delete[] pData;
    maxDataSize = _maxDataSize;
    internalAllocation = true;
    currentDataSize  = 0;	
    pData = new uint8_t[maxDataSize];
}

rawData::~rawData() {
    if (internalAllocation)
        delete[] pData;
}

bool rawData::wasInternalAlloc()
{
    return internalAllocation;
}

#ifdef __DEBUG_MODE__
void rawData::show(std::ostream &os, uint32_t size_words, uint32_t offset_words)
{
    uint32_t* data = (uint32_t*)pData;
    uint32_t  data_nr;
    os << "DATA SIZE: " << currentDataSize/4 << std::endl;
    
    
    if (size_words + offset_words > currentDataSize/4){
        data_nr = currentDataSize/4;
    } else {
        data_nr = size_words + offset_words;
    }
    
    for (unsigned int i = offset_words; i < data_nr; i++){        
        os << "0x" << std::setfill('0') << std::setw(8) << std::uppercase << std::hex << *(data + i) << " " << std::setfill(' ') << std::dec;
        if (i != 0 && !(i % 15))
            os << std::endl;
    }
}

std::ostream& operator<<(std::ostream &os, const rawData& rData)
{
    os << "DATA SIZE: " << rData.currentDataSize << std::endl;
    uint32_t* data = (uint32_t*)rData.pData;
    for (unsigned int i = 0; i < rData.currentDataSize/4; i++){        
        os << "0x" << std::setfill('0') << std::setw(8) << std::uppercase << std::hex << *(data + i) << " " << std::setfill(' ') << std::dec;
        if (i != 0 && !(i % 15))
            os << std::endl;
    }
    return os;
}
#endif /*__DEBUG_MODE__*/
