#include <mtca4u/Device.h>
#include <mtca4u/DeviceBackend.h>
#include <mtca4u/MultiplexedDataAccessor.h>
#include <mtca4u/BackendFactory.h>
#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>

static const std::string MODULE_NAME = "TEST";
static const std::string MAP_NAME = "../tests/muxedDataAcessor.map";
static const std::string DATA_REGION_NAME = "DMA";
static const uint DATA_REGION_SIZE_IN_BYTES = 128;
static const std::string REGISTER_TO_SETUP_DMA_REGION = "AREA_DMAABLE";
static const uint totalNumElementsInAllSequences = 64;

int main() {
  // open the device:
  boost::shared_ptr< mtca4u::Device > device (new mtca4u::Device());
  device->open("PCIE3");
  /** Entry in dmap file is
   *  PCIE3  sdm://./pci:mtcadummys0; muxedDataAcessor.map
   */
  
  // populate a memory region with multiple sequences so that we can use this
  // for demonstrating the use of the MultiplexedDataAccessor. It is important
  // to note that the MultiplexedDataAccessor expects all sequences to be of
  // equal lengths. We are going to set up a region with 16 sequences, each
  // having 4 values. These sequences
  // would be:
  //
  // sequence 0   sequence 1   sequence 2   sequence 3   sequence 4  sequence 5
  // 1	          2            3            4            5           6
  // 17           18           19           20           21          22
  // 33           34           35           36           37          38
  // 49           50           51           52           53          54
  // sequence 6   sequence 7   sequence 8   sequence 9   sequence 10 sequence 11
  // 7            8            9            10           11          12
  // 23           24           25           26           27          28
  // 39           40           41           42           43          44
  // 55           56           57           58           59          60
  // sequence 12  sequence 13  sequence 14  sequence 15
  // 13           14           15           16
  // 29           30           31           32
  // 45           46           47           48
  // 61           62           63           64
  //
  // Each value of this sequence is 2 bytes long. And these sequences would
  // require region that spans 128 bytes (64 elements * 2). So we are going to
  // multiplex these sequences and put this muxed stream/data into such a
  // region. The region is named 'DMA'; The fact that this region contains
  // multiplexed data sequences is indicated by the keyword prefix
  // AREA_MULTIPLEXED_SEQUENCE_. The memory block  named 'DMA' would then look
  // like this(once set up):
  //            [1, 64] -> with each element occupying 2 bytes

  // To populate the above described region named 'DMA' (That would hold muxed
  // sequences as indicated by the prefix AREA_MULTIPLEXED_SEQUENCE_), Making
  // use of a hack. We can have write access to this region through a register
  // AREA_DMAABLE on the dummyDriver PCIE device.
  mtca4u::RegisterInfoMap::RegisterInfo info;
  device->getRegisterMap()->getRegisterInfo(REGISTER_TO_SETUP_DMA_REGION, info);
  // frame a buffer with the muxed data [1, 64], which will be used to populate
  // the DMA region
  std::vector<uint16_t> ioBuffer(totalNumElementsInAllSequences);
  for (size_t i = 0; i < ioBuffer.size(); ++i) {
    ioBuffer[i] = i + 1;
  }

  // set up the 'DMA' region with the hack so that we can demonstrate the
  // DemultiplexedDataAccessor functionality
  device->writeArea(info.address,
                      reinterpret_cast<int32_t*>(&(ioBuffer[0])),
                      DATA_REGION_SIZE_IN_BYTES, info.bar);

  /**********************************************************************/
  // Start of Real Example, now that DMA region is set up with multiplexed
  // sequences
  //boost::shared_ptr< mtca4u::Device< mtca4u::DeviceBackend > > myDevice =
  //	FactoryInstance.createBackend("PCIE3");
  boost::shared_ptr<mtca4u::Device> myDevice( new mtca4u::Device());
  myDevice->open("PCIE3");
  // The 16 bit elements in the 'DMA' region are converted into double because
  // we specify the userType as double. Please note, it is valid to use another
  // data types as the userType as well. For example using
  // MultiplexedDataAccessor<uint16> would convert the read in values from the
  // 'DMA' region to uint_16  (With the fixed point conversion applied)
  boost::shared_ptr<mtca4u::MultiplexedDataAccessor<double> >
  dataDemuxedAsDouble =
      myDevice
          ->getCustomAccessor<mtca4u::MultiplexedDataAccessor<double> >(
               DATA_REGION_NAME,
               MODULE_NAME); // The DATA_REGION_NAME -> 'DMA' is described in
                             // the map file as AREA_MULTIPLEXED_SEQUENCE_DMA
                             // (where AREA_MULTIPLEXED_SEQUENCE_ is the keyword
  // representing the information that the memory region which is named 'DMA'
  // holds multiplexed data sequences)

  // read the memory region 'DMA' using the accessor
  dataDemuxedAsDouble->read();

  // Return the number of sequences found: should be 16
  uint numberOfDataSequences = dataDemuxedAsDouble->getNumberOfDataSequences();
  std::cout << "Number Of dataSequences extracted: " << numberOfDataSequences
            << std::endl;

  // The accessor expects that all sequences are of the same length, and that a
  // described region should have at least one sequence.
  uint lengthOfaSequence = (*dataDemuxedAsDouble)[0].size();
  std::cout << "Length of each sequence: " << lengthOfaSequence << std::endl;

  // The demultiplexed sequences can be displayed:
  for (uint rowCount = 0; rowCount < numberOfDataSequences; ++rowCount) {
    std::cout << "Sequence: " << rowCount << std::endl;
    for (uint columnCount = 0; columnCount < lengthOfaSequence; ++columnCount) {
      // Here value returned is a double.
      std::cout << (*dataDemuxedAsDouble)[rowCount][columnCount] << std::endl;
    }
    std::cout << std::endl;
  }

  // Modify a value and write it back
  //
  // dataDemuxedAsDouble[1][0] = 5;
  // dataDemuxedAsDouble->write()
  //
  // The above code does not work for memory regions which uses DMA transfer. So
  // for this example, where the underlying memory region named 'DMA', uses DMA
  // transfers for populating information from the card, write would throw up an
  // exception saying functionality not implemented yet. How ever for regions
  // that do not use DMA transfers to access the hardware, The above commented
  // out code snippet would work.

  // It is good style to close the device when you are done, although
  // this would happen automatically once the device goes out of scope.
  myDevice->close();

  return 0;
}
