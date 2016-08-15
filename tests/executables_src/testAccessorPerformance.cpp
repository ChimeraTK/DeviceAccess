#include <chrono>

#include "Device.h"
#include "Utilities.h"

using namespace mtca4u;
using namespace std::chrono;

int main(){
  steady_clock::time_point t0;
  duration<double> tdur;

  setDMapFilePath("dummies.dmap");

  Device device;
  device.open("PERFTEST");

  int niter = 100000;
  int niterBlock = 1000;

  int64_t sum = 0;

  std::cout <<" ***************************************************************************" << std::endl;
  std::cout <<" Tests with the OneDRegisterAccessor:" << std::endl;

  auto acc1D = device.getOneDRegisterAccessor<int>("ADC/AREA_DMA_VIA_DMA");
  t0 = steady_clock::now();
  std::cout <<" reading block ";
  for(int i= 0; i < niterBlock; ++i){
      acc1D.read();
      sum += acc1D[i];
  }
  tdur = steady_clock::now()-t0;
  std::cout << "took " << tdur.count()*1000./niterBlock << " ms per block" << std::endl;

  auto acc1Draw = device.getOneDRegisterAccessor<int>("ADC/AREA_DMA_VIA_DMA",0,0, {AccessMode::raw});
  t0 = steady_clock::now();
  std::cout <<" raw-reading block ";
  for(int i= 0; i < niterBlock; ++i){
    acc1Draw.read();
      sum += acc1Draw[i];
  }
  tdur = steady_clock::now()-t0;
  std::cout << "took " << tdur.count()*1000./niterBlock << " ms per block" << std::endl;\

  return 0;

  std::cout <<" ***************************************************************************" << std::endl;
  std::cout <<" Tests with the compatibility RegisterAccessor:" << std::endl;

  auto accessor = device.getRegisterAccessor("WORD_STATUS","BOARD");

  std::cout <<" reading ";
  sum += accessor->read<int>();
  t0 = steady_clock::now();
  for(int i= 0; i < niter; ++i){
    sum += accessor->read<int>();
  }
  tdur = steady_clock::now()-t0;
  std::cout << "took " << tdur.count()*1000000./niter << " us per word" << std::endl;

  std::cout <<" writing ";
  t0 = steady_clock::now();
  for(int i= 0; i < niter; ++i){
    accessor->write<int>(i);
  }
  tdur = steady_clock::now()-t0;
  std::cout << "took " << tdur.count()*1000000./niter << " us per word" << std::endl;

  std::cout <<" reading raw ";
  t0 = steady_clock::now();
  for(int i= 0; i < niter; ++i){
    int buffer;
    accessor->readRaw(&buffer,sizeof(int));
    sum += buffer;
  }
  tdur = steady_clock::now()-t0;
  std::cout << "took " << tdur.count()*1000000./niter << " us per word" << std::endl;

  std::cout <<" writing raw ";
  t0 = steady_clock::now();
  for(int i= 0; i < niter; ++i){
    accessor->writeRaw(&i, sizeof(int));
  }
  tdur = steady_clock::now()-t0;
  std::cout << "took " << tdur.count()*1000000./niter << " us per word" << std::endl;

  auto accessor2 = device.getRegisterAccessor("AREA_DMAABLE","ADC");
  t0 = steady_clock::now();
  std::cout <<" reading moving word in block ";
  for(int i= 0; i < niterBlock; ++i){
      int buffer;
      accessor2->read<int>(&buffer, 1, i);
      sum += buffer;
  }
  tdur = steady_clock::now()-t0;
  std::cout << "took " << tdur.count()*1000./niterBlock << " ms per transfer" << std::endl;


  std::cout <<" ***************************************************************************" << std::endl;
  t0 = steady_clock::now();
  std::cout << " Sum of all read data: " << sum << std::endl;
  tdur = steady_clock::now()-t0;
  std::cout << " Printing the sum took " << tdur.count()*1000. << " ms" << std::endl;
  std::cout <<" ***************************************************************************" << std::endl;

  return 0;
}
