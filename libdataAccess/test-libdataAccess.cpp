#include "libdataAccess.h"

#include <iostream>
#include <exception>
#include <string.h>
#include <iostream>

#define DATA_SIZE 100

int main()
{
    try {        
        dataAccess            da;
        rawData               wData(DATA_SIZE);
        metaData              mData;
        rawData               bDataRaw1(30000);
                
        da.addProtocol(new dataProtocolDOOCS());
        da.addProtocol(new dataProtocolPCIE("demo_devMapFile.dmap"));
        da.addProtocol(new dataProtocolRemapBuffer());
        da.addProtocol(new dataProtocolAlias());
        da.init("./demo_logicNameMapperFile.lmap");
       
        std::cout << da << std::endl;
        
     //   da.readMetaData(dataAccess::DATA_CHANNEL_LEVEL, "TEST", "DATA_CHANNEL_INFO", mData);
     //   std::cout << mData << std::endl;
        
     //   da.readMetaData(dataAccess::PROTOCOL_LEVEL, "TEST_X1", "MAP:HW_VERSION", mData);
     //   std::cout << mData << std::endl;
  /*       
        dataProtocolElem      *dap;
        dap = da.getDeviceObject("TEST");
        dap->readMetaData("DATA_CHANNEL_INFO", mData);        
        std::cout << mData << std::endl;
        
        return 0;
      
        dataProtocolRemapBuffer*   dpb_1 = new dataProtocolRemapBuffer;
        
        
        dpb_1->addBuffer("AREA_DMA_11", &bDataRaw1);
        dpb_1->addBuffer("AREA_DMA_1", &bDataRaw1);
        dpb_1->addBuffer("AREA_I_11", &bDataRaw1);
        dpb_1->addBuffer("AREA_Q_11", &bDataRaw1);
        dpb_1->addBuffer("AREA_A_11", &bDataRaw1);
        dpb_1->addBuffer("AREA_P_11", &bDataRaw1);
         
        da.addProtocol(dpb_1);
        da.init("./demo_logicNameMapperFile.lmap");
        
        std::cout << da << std::endl;
        //return 0;
        
        da.readData("CAV_A0_SLOW_ADC11", wData);
        std::cout << wData << std::endl;
        
        da.readData("TIMING_ADC_1", wData);
        std::cout << wData << std::endl;
        da.readData("TIMING_ADC_PART_1", wData);
        std::cout << wData << std::endl;
        da.readData("TIMING_ADC_PART_2", wData);
        std::cout << wData << std::endl;        
        da.readData("FIRMWARE_COMPILATION_1", wData);
        std::cout << wData << std::endl;
        
        
        dataProtocolElem      *dap;
        dap = da.getDeviceObject("CAV_A0_SLOW_ADC");
        dap->readData(wData);
        std::cout << wData << std::endl;
        
        dap->readMetaData("", iData);
       
        std::cout << iData << std::endl;
        std::cout << dap->getAddress() << std::endl;
        
        dataProtocolRemapBuffer*   dpb = new dataProtocolRemapBuffer;
        dpb->addBuffer("AREA_I_1", &bDataRaw1);
        dpb->addBuffer("AREA_P_1", &bDataRaw1);
        da.addProtocol(dpb);
        
        rawData               data(30000);        
        for (int i = 0; i < 30000/4; i++){
            *(((int*)data.pData) + i) = i;
        }
        da.writeData("AREA_DMA_1", data);                                                              
        
        da.readData("AREA_DMA_1", bDataRaw1);  
        da.readData("P_DATA_1", bDataRaw1);  
        rawData               bData;
        da.readData("I_DATA_1", bData);
        //std::cout << bData << std::endl;
        
        rawData test(4);
        *((int*)test.pData) = 7;
        da.writeData("DATA", test);
        da.readData("DATA", test);
        std::cout << "TEST: " << test << std::endl;
        da.readData("TIMING_ADC_PART_1", test);
        std::cout << "TEST: " << test << std::endl;
        
        
        std::cout << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << std::endl;
        
        std::cout << da << std::endl;
        
        
        return 0;
        
   
        dataProtocolElem      *dap;
        
        
        
               
        

        dap = da.getDeviceObject("I_CAV_1");
        
        std::cout << da << std::endl;
        
        uint32_t* data = (uint32_t*)wData.pData;
        for (int i = 0; i < DATA_SIZE/4; i++){
            *(data + i) = i;
        }

        da.writeData("I_CAV_1", wData);                

        memset(rData.pData, 0, DATA_SIZE);                
        da.readData("I_CAV_1", rData);
        std::cout << rData << std::endl;                                                
        

        dap->writeData(wData);  

        memset(rData.pData, 0, DATA_SIZE);                
        dap->readData(rData);
        std::cout << rData << std::endl;
        
        dap->readInfo(iData);
        std::cout << iData.info << std::endl;
     */           

    } catch (std::exception &ex){
        std::cerr <<  ex.what() << std::endl;
    } catch (...){
        std::cerr << "Unknown exception" << std::endl;
    }
    return 0;
}
