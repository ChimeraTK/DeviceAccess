#ifndef CHIMERA_TK_DUMMY_BACKEND_H
#define CHIMERA_TK_DUMMY_BACKEND_H

#include <list>
#include <map>
#include <mutex>
#include <set>
#include <vector>

#include <boost/function.hpp>

#include "Exception.h"
#include "DummyBackendBase.h"
#include "RegisterInfoMap.h"

namespace ChimeraTK {

  // foward declarations
  template<typename T>
  class DummyRegisterAccessor;

  template<typename T>
  class DummyMultiplexedRegisterAccessor;

  class DummyRegisterRawAccessor;

  /** The dummy device opens a mapping file instead of a device, and
   *  implements all registers defined in the mapping file in memory.
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
  class DummyBackend : public DummyBackendBase<DummyBackend> {
   public:
    DummyBackend(std::string mapFileName);
    ~DummyBackend() override;

    void open() override;

    /** This closes the device, clears all internal registers, read-only settings
     * and callback functions. As the device could be opened with another mapping
     * file later, these will most probably be invalid in this case. This is why
     * the read-only  settings and callback functions have to be set again when
     * reopening the file.
     */
    void close() override;

    using DummyBackendBase<DummyBackend>::read;  // use the 32 bit version from the base class
    using DummyBackendBase<DummyBackend>::write; // use the 32 bit version from the base class
    void read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) override;
    void write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) override;

    bool isFunctional() const override { return (_opened && !_hasActiveException); }

    std::string readDeviceInfo() override;

    static boost::shared_ptr<DeviceBackend> createInstance(
        std::string address, std::map<std::string, std::string> parameters);

    /** Get a raw accessor to the underlying memory with the convenience of using 
     *  register names. This accessor had nothing to do with regular, TransferElement based
     *  accessors and serves as second, independet implementation for debugging.
     *
     *  We have to use the old module/register interface because the dummy uses the old style
     *  mapping internally.
     */
    DummyRegisterRawAccessor getRawAccessor(std::string module, std::string register_name);

    void setException() override { _hasActiveException = true; }

   protected:
    struct AddressRange {
      const uint64_t offset;
      const uint32_t sizeInBytes;
      const uint64_t bar;
      AddressRange(uint64_t bar_, uint64_t address, size_t sizeInBytes_)
      : offset(address), sizeInBytes(sizeInBytes_), bar(bar_) {}
      bool operator<(AddressRange const& right) const {
        return (bar == right.bar ? (offset < right.offset) : bar < right.bar);
      }
    };

    /** name of the map file */
    std::string _mapFile;

    std::map<uint64_t, std::vector<int32_t>> _barContents;
    std::set<std::pair<uint64_t, uint64_t>> _readOnlyAddresses; // bar/address combinations which are read only
    std::multimap<AddressRange, boost::function<void(void)>> _writeCallbackFunctions;
    std::mutex mutex;

    std::atomic<bool> _hasActiveException{false};

    void resizeBarContents();

    void runWriteCallbackFunctionsForAddressRange(AddressRange addressRange);
    std::list<boost::function<void(void)>> findCallbackFunctionsForAddressRange(AddressRange addressRange);
    void setReadOnly(uint64_t bar, uint64_t address, size_t sizeInWords);
    void setReadOnly(AddressRange addressRange);
    bool isReadOnly(uint64_t bar, uint64_t address) const;
    void setWriteCallbackFunction(AddressRange addressRange, boost::function<void(void)> const& writeCallbackFunction);
    /// returns true if the ranges overlap and at least one of the overlapping
    /// registers can be written
    bool isWriteRangeOverlap(AddressRange firstRange, AddressRange secondRange);

    /// Not write-protected function for internal use only. It does not trigger
    /// the callback function so it can be used inside a callback function for
    /// resynchronisation.
    void writeRegisterWithoutCallback(uint64_t bar, uint64_t address, int32_t data);

    /** map of instance names and pointers to allow re-connecting to the same
     * instance with multiple Devices */
    static std::map<std::string, boost::shared_ptr<ChimeraTK::DeviceBackend>>& getInstanceMap() {
      static std::map<std::string, boost::shared_ptr<ChimeraTK::DeviceBackend>> instanceMap;
      return instanceMap;
    }
    // DummyBackendBase needs access to getInstanceMap()
    friend class DummyBackendBase<DummyBackend>;

    /// register accessors must be friends to access the map and the registers
    template<typename T>
    friend class DummyRegisterAccessor;

    template<typename T>
    friend class DummyMultiplexedRegisterAccessor;

    friend class DummyRegisterRawAccessor;
    friend class SharedDummyBackend;

    static std::string convertPathRelativeToDmapToAbs(std::string const& mapfileName);
  };

} // namespace ChimeraTK

#endif // CHIMERA_TK_DUMMY_BACKEND_H
