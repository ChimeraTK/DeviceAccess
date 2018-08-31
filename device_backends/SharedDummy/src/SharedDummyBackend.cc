#include <algorithm>
#include <sstream>
#include <functional>

#include <boost/lambda/lambda.hpp>
#include <boost/filesystem.hpp>

#include "SharedDummyBackend.h"
#include "Exception.h"
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
      throw ChimeraTK::logic_error(errorMessage.str());\
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
    _registerMapping(_registerMap),
    _barSizesInBytes(getBarSizesInBytesFromRegisterMapping()),
    sharedMemoryManager(*this, instanceId, mapFileName)
  {
    // Note: Opposed to the other dummies, _registerMap is computed in the base class ctor
    //       because we rely on a fixed init-order for the boost::interprocess members
    setupBarContents();
  }

  //Nothing to clean up, all objects clean up for themselves when
  //they go out of scope.
  SharedDummyBackend::~SharedDummyBackend(){
  }

  // Construct a segment for each bar and set required size
  void SharedDummyBackend::setupBarContents(){

    for (std::map< uint8_t, size_t >::const_iterator barSizeInBytesIter
           = _barSizesInBytes.begin();
         barSizeInBytesIter != _barSizesInBytes.end();
         ++barSizeInBytesIter){

      std::string barName = SHARED_MEMORY_BAR_PREFIX + std::to_string(barSizeInBytesIter->first);

      size_t barSizeInWords = (barSizeInBytesIter->second + sizeof(int32_t) - 1)/sizeof(int32_t);

      try{
        std::lock_guard<boost::interprocess::named_mutex> lock(sharedMemoryManager.interprocessMutex);
        _barContents[barSizeInBytesIter->first] = sharedMemoryManager.findOrConstructVector(barName, barSizeInWords);
      }
      catch(boost::interprocess::bad_alloc &e){
        // Clean up
        sharedMemoryManager.~SharedMemoryManager();

        std::string errMsg{"Could not allocate shared memory while constructing registers. "
                           "Please file a bug report at https://github.com/ChimeraTK/DeviceAccess."};
        throw ChimeraTK::logic_error(errMsg);
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
      throw ChimeraTK::logic_error("Device is already open.");
    }
    _opened=true;
  }

  void SharedDummyBackend::close(){
    if (!_opened){
      throw ChimeraTK::logic_error("Device is already closed.");
    }
    _opened=false;
  }

  void SharedDummyBackend::read(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes){
    if (!_opened){
      throw ChimeraTK::logic_error("Device is closed.");
    }
    checkSizeIsMultipleOfWordSize( sizeInBytes );
    unsigned int wordBaseIndex = address/sizeof(int32_t);

    std::lock_guard<boost::interprocess::named_mutex> lock(sharedMemoryManager.interprocessMutex);

    for (unsigned int wordIndex = 0; wordIndex < sizeInBytes/sizeof(int32_t); ++wordIndex){
      TRY_REGISTER_ACCESS( data[wordIndex] = _barContents[bar]->at(wordBaseIndex+wordIndex); );
    }
  }

  void SharedDummyBackend::write(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes){
    if (!_opened){
      throw ChimeraTK::logic_error("Device is closed.");
    }
    checkSizeIsMultipleOfWordSize( sizeInBytes );
    unsigned int wordBaseIndex = address/sizeof(int32_t);

    std::lock_guard<boost::interprocess::named_mutex> lock(sharedMemoryManager.interprocessMutex);

    for (unsigned int wordIndex = 0; wordIndex < sizeInBytes/sizeof(int32_t); ++wordIndex){
      TRY_REGISTER_ACCESS( _barContents[bar]->at(wordBaseIndex+wordIndex) = data[wordIndex]; );
    }
  }

  std::string SharedDummyBackend::readDeviceInfo(){
    std::stringstream info;
    info << "SharedDummyBackend with mapping file " << _registerMapping->getMapFileName();
    return info.str();
  }


  size_t SharedDummyBackend::getTotalRegisterSizeInBytes() const{
    size_t totalRegSize = 0;
    for(auto &pair : _barSizesInBytes) {
      totalRegSize += pair.second;
    }
    return totalRegSize;
  }



  void SharedDummyBackend::checkSizeIsMultipleOfWordSize(size_t sizeInBytes){
    if (sizeInBytes % sizeof(int32_t) ){
      throw ChimeraTK::logic_error("Read/write size has to be a multiple of 4");
    }
  }

  boost::shared_ptr<DeviceBackend> SharedDummyBackend::createInstance(std::string /*host*/,
      std::string instance,
      std::list<std::string> parameters,
      std::string /*mapFileName*/) {

    std::string mapFileName = parameters.front();
    if(mapFileName == "") {
      throw ChimeraTK::logic_error("No map file name given.");
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
    boost::filesystem::path absPathToMapFile{parserUtilities::concatenatePaths(absPathToDmapDir, mapfileName)};
    // Possible ./, ../ elements are removed, as the path may be constructed differently
    // in different client applications
    return boost::filesystem::canonical(absPathToMapFile).string();
  }


} // Namespace ChimeraTK
