#include <algorithm>
#include <sstream>
#include <functional>

#include <boost/lambda/lambda.hpp>

#include "SharedDummyBackend.h"
#include "NotImplementedException.h"
#include "MapFileParser.h"
#include "parserUtilities.h"
#include "BackendFactory.h"

//macro to avoid code duplication
#define TRY_REGISTER_ACCESS(COMMAND)\
    try{\
      COMMAND\
    }catch( std::out_of_range & outOfRangeException ){\
      std::stringstream errorMessage;\
      errorMessage << "Invalid address offset " << address \
      << " in bar " << static_cast<int>(bar) << "."	\
      << "Caught out_of_range exception: " << outOfRangeException.what();\
      throw DeviceException(errorMessage.str(), DeviceException::WRONG_PARAMETER);\
    }


namespace ChimeraTK {
  // Valid bar numbers are 0 to 5 , so they must be contained
  // in three bits.
  const unsigned int BAR_MASK = 0x7;
  // the bar number is stored in bits 60 to 62
  const unsigned int BAR_POSITION_IN_VIRTUAL_REGISTER = 60;

  SharedDummyBackend::SharedDummyBackend(std::string instanceId, std::string mapFileName)
  : NumericAddressedBackend(mapFileName),
    _mapFile(mapFileName),
    //TODO Nasty to use base class member as argument here?
    _registerMapping(_registerMap),
    _barSizes(getBarSizesInWords()),
    sharedMemoryManager(instanceId, mapFileName, getTotalRegisterSizeInWords()*sizeof(int32_t))
  {
    // Note: Opposed to the other dummies, _registerMap is computed in the base class ctor
    //       because we rely on a fixed init-order for the boost::interprocess members
    //_registerMap = MapFileParser().parse(_mapFile);
    //_registerMapping = _registerMap;

    // TODO Moved to helper routines
//    // compute bar sizes in words
//    for(auto &reg : *_registerMapping) {
//      size_t regSize = static_cast<size_t> (reg.address + reg.nBytes + sizeof(int32_t) - 1 ) / sizeof(int32_t);   // always round up
//      _barSizes[reg.bar] = std::max( _barSizes[reg.bar], regSize );
//    }
//
    // compute total bar size in words
//    size_t totalRegSize = 0;
//    for(auto &pair : _barSizes) {
//      totalRegSize += pair.second;
//    }


    // TODO: setup cannot be performed in the manager's ctor, because we depend on totalRegSize being known
    //       Is there a better way to do this? -> Might move _barSizes computation to shdMemManager, it is only used there.
    //sharedMemoryManager.setup(totalRegSize*sizeof(int32_t));

    // TODO Define barSizesInBytes. Why had this been moved from the separate routine?
    //             is this necessary at all, we got the size in words in totalRegSize and call shm.setup with totRegSize*sizeof(int32)
    // Note: In the normal dummy backend, this also allocates the memory through .resize()
    //       -> Cannot use this here as we allocate in shared mem.
//    for (std::map< uint8_t, size_t >::const_iterator barSizeInBytesIter
//        = barSizesInBytes.begin();
//        barSizeInBytesIter != barSizesInBytes.end(); ++barSizeInBytesIter){
//
//      //the size of the vector is in words, not in bytes -> convert fist
//      _barContents[barSizeInBytesIter->first].resize(
//          barSizeInBytesIter->second / sizeof(int32_t), 0);
//    }
    
    resizeBarContents();

  }

  //Nothing to clean up, all objects clean up for themselves when
  //they go out of scope.
  SharedDummyBackend::~SharedDummyBackend(){
  }

  // Construct a segment for each bar and set required size
  void SharedDummyBackend::resizeBarContents(){
    std::map< uint8_t, size_t > barSizesInBytes
    = getBarSizesInBytesFromRegisterMapping();

    for (std::map< uint8_t, size_t >::const_iterator barSizeInBytesIter
           = barSizesInBytes.begin();
         barSizeInBytesIter != barSizesInBytes.end();
         ++barSizeInBytesIter){

      std::string barName = "Bar"+std::to_string(barSizeInBytesIter->first);

      try{
        _barContents[barSizeInBytesIter->first]
            = sharedMemoryManager.segment.find_or_construct<MyVector>(barName.c_str())(sharedMemoryManager.alloc_inst);

        size_t barSizeInBytes = barSizeInBytesIter->second / sizeof(int32_t);

        //the size of the vector is in words, not in bytes -> convert fist
        _barContents[barSizeInBytesIter->first]->resize(
            barSizeInBytes, 0);
#ifdef _DEBUG
        std::cout << "Constructed bar: " << barName << std::endl
                  << " with a size of " << barSizeInBytes << std::endl << std::flush;
#endif
      }
      catch(boost::interprocess::bad_alloc &e){\
        // TODO Decide if we trigger grow() on the shd mem from here,
        // but this requires that the shd mem is unmapped in all processes.
        std::cout << "Caught boost::interprocess::bad_alloc when constructing?resizing " + barName
                  << std::endl << std::flush;
      }
    } /* for(barSizesInBytesIter) */
  }

  std::map< uint8_t, size_t > SharedDummyBackend::getBarSizesInBytesFromRegisterMapping() const{
    std::map< uint8_t, size_t > barSizesInBytes;
    for (RegisterInfoMap::const_iterator mappingElementIter = _registerMapping->begin();
        mappingElementIter !=  _registerMapping->end(); ++mappingElementIter ) {
      barSizesInBytes[mappingElementIter->bar] =
          std::max( barSizesInBytes[mappingElementIter->bar],
              static_cast<size_t> (mappingElementIter->address +
                  mappingElementIter->nBytes ) );
    }
    return barSizesInBytes;
  }

  void SharedDummyBackend::open()
  {
    if (_opened){
      throw DeviceException("Device is already open.", DeviceException::CANNOT_OPEN_DEVICEBACKEND);
    }
    _opened=true;
  }

  void SharedDummyBackend::close(){
    if (!_opened){
      throw DeviceException("Device is already closed.", DeviceException::NOT_OPENED);
    }
    _opened=false;
  }

  void SharedDummyBackend::read(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes){
    if (!_opened){
      throw DeviceException("Device is closed.", DeviceException::NOT_OPENED);
    }
    checkSizeIsMultipleOfWordSize( sizeInBytes );
    unsigned int wordBaseIndex = address/sizeof(int32_t);
    for (unsigned int wordIndex = 0; wordIndex < sizeInBytes/sizeof(int32_t); ++wordIndex){
      TRY_REGISTER_ACCESS( data[wordIndex] = _barContents[bar]->at(wordBaseIndex+wordIndex); );
    }
  }

  void SharedDummyBackend::write(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes){
    if (!_opened){
      throw DeviceException("Device is closed.", DeviceException::NOT_OPENED);
    }
    checkSizeIsMultipleOfWordSize( sizeInBytes );
    unsigned int wordBaseIndex = address/sizeof(int32_t);
    for (unsigned int wordIndex = 0; wordIndex < sizeInBytes/sizeof(int32_t); ++wordIndex){
      TRY_REGISTER_ACCESS( _barContents[bar]->at(wordBaseIndex+wordIndex) = data[wordIndex]; );
    }
  }

  std::string SharedDummyBackend::readDeviceInfo(){
    std::stringstream info;
    info << "SharedDummyBackend with mapping file " << _registerMapping->getMapFileName();
    return info.str();
  }


  std::map<uint8_t, size_t> SharedDummyBackend::getBarSizesInWords(){

    std::map<uint8_t, size_t> barSizesTemp;

    for(auto &reg : *_registerMapping) {
      size_t regSize = static_cast<size_t> (reg.address + reg.nBytes + sizeof(int32_t) - 1 ) / sizeof(int32_t);   // always round up
      barSizesTemp[reg.bar] = std::max( barSizesTemp[reg.bar], regSize );
    }
    return barSizesTemp;
  }

  size_t SharedDummyBackend::getTotalRegisterSizeInWords(){
    size_t totalRegSize = 0;
    for(auto &pair : _barSizes) {
      totalRegSize += pair.second;
    }
    return totalRegSize;
  }



  void SharedDummyBackend::checkSizeIsMultipleOfWordSize(size_t sizeInBytes){
    if (sizeInBytes % sizeof(int32_t) ){
      throw( DeviceException("Read/write size has to be a multiple of 4", DeviceException::WRONG_PARAMETER) );
    }
  }


  boost::shared_ptr<DeviceBackend> SharedDummyBackend::createInstance(std::string /*host*/,
      std::string instance,
      std::list<std::string> parameters,
      std::string /*mapFileName*/) {

    std::string mapFileName = parameters.front();
    if(mapFileName == "") {
      throw ChimeraTK::DeviceException("No map file name given.", ChimeraTK::DeviceException::WRONG_PARAMETER);
    }

    // when the factory is used to create the dummy device, mapfile path in the
    // dmap file is relative to the dmap file location. Converting the relative
    // mapFile path to an absolute path avoids issues when the dmap file is not
    // in the working directory of the application.
    return returnInstance<SharedDummyBackend>(instance, instance, convertPathRelativeToDmapToAbs(mapFileName));
  }

  std::string SharedDummyBackend::convertPathRelativeToDmapToAbs(const std::string& mapfileName) {
    std::string dmapDir = parserUtilities::extractDirectory( BackendFactory::getInstance().getDMapFilePath());
    std::string absPathToDmapDir = parserUtilities::convertToAbsolutePath(dmapDir);
    // the map file is relative to the dmap file location. Convert the relative
    // mapfilename to an absolute path
    return parserUtilities::concatenatePaths(absPathToDmapDir, mapfileName);
  }

} // namespace mtca4u
