#ifndef MTCA4U_DUMMY_BACKEND_H
#define MTCA4U_DUMMY_BACKEND_H

#include <vector>
#include <map>
#include <list>
#include <set>

#include <boost/function.hpp>

#include "Exception.h"
#include "RegisterInfoMap.h"
#include "NumericAddressedBackend.h"

namespace mtca4u {

  /** To provide exception for DummyBackend.
   *
   */
  class DummyBackendException : public DeviceBackendException {
    public:
      enum {WRONG_SIZE, ALREADY_OPEN, ALREADY_CLOSED, INVALID_ADDRESS, INVALID_PARAMETER};
      DummyBackendException(const std::string &message, unsigned int exceptionID)
      : DeviceBackendException( message, exceptionID ){}
  };


  // foward declarations
  template<typename T> class DummyRegisterAccessor;
  template<typename T> class DummyMultiplexedRegisterAccessor;

  /** The dummy device opens a mapping file instead of a device, and
   *  implements all registers defined in the mapping file im memory.
   *  Like this it mimiks the real PCIe device.
   *
   *  Deriving from this class, you can write dedicated implementations
   *  with special functionality.
   *  For this purpose one can register write callback function which are
   *  executed if a certain register (or range of registers) is written.
   *  For instance: Writing to a START_DAQ register
   *  can fill a data buffer with dummy values which can be read back.
   *  For each call of writeReg or writeArea the callback function is called once.
   *  If you require the callback function to be executed after each
   *  register change, use writeReg multiple times instead of writeArea.
   *
   *  Registers can be set to read-only mode. In this
   *  case a write operation will just be ignored and no callback
   *  function is executed.
   */
  class DummyBackend : public NumericAddressedBackend
  {
    public:
      DummyBackend(std::string mapFileName);
      virtual ~DummyBackend();

      virtual void open();

      /** This closes the device, clears all internal regsiters, read-only settings and
       *  callback functions. As the device could be opened with another mapping file later,
       *  these will most probably be invalid in this case. This is why the read-only  settings
       *  and callback functions have to be set again when reopening the file.
       */
      virtual void close();

      virtual void read(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes);
      virtual void write(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes);

      virtual std::string readDeviceInfo();

      /// A virtual address is an address is a virtual 64 bit address space
      /// which contains all bars.
      static uint64_t calculateVirtualAddress(
          uint32_t registerOffsetInBar,
          uint8_t bar);

      static boost::shared_ptr<DeviceBackend> createInstance(std::string host, std::string instance, std::list<std::string> parameters, std::string mapFileName);

    protected:

      struct AddressRange{
        const uint32_t offset;
        const uint32_t sizeInBytes;
        const uint8_t bar;
        AddressRange( uint8_t bar_, uint32_t address,  size_t sizeInBytes_ )
        : offset( address ), sizeInBytes( sizeInBytes_ ), bar( bar_ ){}
        bool operator<(AddressRange const & right) const {
          return ( bar == right.bar ? ( offset < right.offset ) : bar < right.bar );
        }
      };

      /** name of the map file */
      std::string _mapFile;

      std::map< uint8_t, std::vector<int32_t> > _barContents;
      std::set< uint64_t > _readOnlyAddresses;
      std::multimap< AddressRange, boost::function<void(void)> > _writeCallbackFunctions;
      RegisterInfoMapPointer _registerMapping;

      void resizeBarContents();
      std::map< uint8_t, size_t > getBarSizesInBytesFromRegisterMapping() const;
      void runWriteCallbackFunctionsForAddressRange( AddressRange addressRange );
      std::list< boost::function<void(void)> > findCallbackFunctionsForAddressRange(AddressRange addressRange);
      void setReadOnly( uint8_t bar, uint32_t address, size_t sizeInWords);
      void setReadOnly( AddressRange addressRange);
      bool isReadOnly( uint8_t bar, uint32_t address ) const;
      void setWriteCallbackFunction( AddressRange addressRange,
          boost::function<void(void)>  const & writeCallbackFunction );
      /// returns true if the ranges overlap and at least one of the overlapping registers can be written
      bool isWriteRangeOverlap( AddressRange firstRange, AddressRange secondRange);
      static void checkSizeIsMultipleOfWordSize(size_t sizeInBytes);

      /// Not write-protected function for internal use only. It does not trigger
      /// the callback function so it can be used inside a callback function for
      /// resynchronisation.
      void writeRegisterWithoutCallback(uint8_t bar, uint32_t address, int32_t data);

      /** map of instance names and pointers to allow re-connecting to the same instance with multiple Devices */
      static std::map< std::string, boost::shared_ptr<mtca4u::DeviceBackend> >& getInstanceMap() {
        static std::map< std::string, boost::shared_ptr<mtca4u::DeviceBackend> > instanceMap;
        return instanceMap;
      }

      /// register accessors must be friends to access the map and the registers
      template<typename T>
      friend class DummyRegisterAccessor;

      template<typename T>
      friend class DummyMultiplexedRegisterAccessor;

      static std::string convertPathRelativeToDmapToAbs(std::string const & mapfileName);

      /**
       * @brief Method looks up and returns an existing instance of class 'T'
       * corresponding to instanceId, if instanceId is a valid  key in the
       * internal map. For an instanceId not in the internal map, a new instance
       * of class T is created, cached and returned. Future calls to
       * returnInstance with this instanceId, returns this cached instance. If
       * the instanceId is "" a new instance of class T is created and
       * returned. This instance will not be cached in the internal memory.
       *
       * @param instanceId Used as key for the object instance look up. "" as
       *                   instanceId will return a new T instance that is not
       *                   cached.
       * @param arguments  This is a template argument list. The constructor of
       *                   the created class T, gets called with the contents of
       *                   the argument list as parameters.
       */
      template <typename T, typename... Args>
      static boost::shared_ptr<DeviceBackend> returnInstance(
          const std::string& instanceId, Args&&... arguments) {
        if (instanceId == "") {
          return boost::shared_ptr<DeviceBackend>(new T(arguments...));
        }
        // search instance map and create new instanceId, if not found under the
        // name
        if (getInstanceMap().find(instanceId) == getInstanceMap().end()) {
          boost::shared_ptr<DeviceBackend> ptr(new T(arguments...));
          getInstanceMap().insert(std::make_pair(instanceId, ptr));
          return ptr;
        }
        // return existing instanceId from the map
        return boost::shared_ptr<DeviceBackend>(getInstanceMap()[instanceId]);
      }

  };

}//namespace mtca4u

#endif // MTCA4U_DUMMY_BACKEND_H
