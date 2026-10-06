// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DummyBackendBase.h"
#include "Exception.h"

#include <boost/function.hpp>

#include <cstddef>
#include <list>
#include <map>
#include <mutex>
#include <set>
#include <vector>

namespace ChimeraTK {

  namespace proxies {
    class RawByteAccess;
  } // namespace proxies

  // forward declarations
  template<typename T>
  class DummyRegisterAccessor;

  template<typename T>
  class DummyMultiplexedRegisterAccessor;

  template<typename RawType>
  class DummyRegisterRawAccessor;

  /** The dummy device opens a mapping file instead of a device, and
   *  implements all registers defined in the mapping file in memory.
   *  Like this it mimics the real PCIe device.
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
   *  Read-only registers from the map file are enforced at the accessor layer,
   *  not in the backend, so a direct write to the backend bypasses them.
   */
  class DummyBackend : public DummyBackendBase {
   public:
    explicit DummyBackend(const std::string& mapFileName, const std::string& dataConsistencyKeyDescriptor = "");
    ~DummyBackend() override = default;

    void open() override;

    /** This closes the device and clears all internal registers and callback functions. As the device could be opened
     *  with another mapping file later, these will most probably be invalid in this case. This is why the callback
     *  functions have to be set again when reopening the file.
     */
    void closeImpl() override;

    using DummyBackendBase::read;  // use the 32 bit version from the base class
    using DummyBackendBase::write; // use the 32 bit version from the base class
    void read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) override;
    void write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) override;

    std::string readDeviceInfo() override;

    static boost::shared_ptr<DeviceBackend> createInstance(
        std::string address, std::map<std::string, std::string> parameters);

    /** Get a raw accessor to the underlying memory with the convenience of using
     *  register names. This accessor had nothing to do with regular, TransferElement based
     *  accessors and serves as second, independent implementation for debugging.
     *
     *  We have to use the old module/register interface because the dummy uses the old style
     *  mapping internally.
     *
     *  The raw type can be chosen through the template parameter; it defaults to int32_t, matching the width of the
     *  historic 32-bit accessor.
     */
    template<typename RawType = int32_t>
    DummyRegisterRawAccessor<RawType> getRawAccessor(const std::string& module, const std::string& register_name) {
      return {shared_from_this(), module, register_name};
    }

    VersionNumber triggerInterrupt(uint32_t interruptNumber) override;

   protected:
    struct AddressRange {
      const uint64_t offset;
      const uint64_t sizeInBytes;
      const uint64_t bar;

      AddressRange(uint64_t bar_, uint64_t address, uint64_t sizeInBytes_)
      : offset(address), sizeInBytes(sizeInBytes_), bar(bar_) {}

      explicit AddressRange(const NumericAddressedRegisterInfo& info)
      : offset(info.address), sizeInBytes(static_cast<uint64_t>(info.nElements) * info.elementPitchBits / 8),
        bar(info.bar) {
        assert(info.elementPitchBits % 8 == 0);
      }

      bool operator<(AddressRange const& right) const {
        return (bar == right.bar ? (offset < right.offset) : bar < right.bar);
      }
    };

    /** name of the map file */
    std::string _mapFile;

    /// The lazily materialised bar contents, keyed by (bar, chunkIndex). Each value is a CHUNK_SIZE big,
    /// zero-initialised chunk.
    std::map<std::pair<uint64_t, uint64_t>, std::vector<std::byte>> _barContents;

    std::multimap<AddressRange, boost::function<void(void)>> _writeCallbackFunctions;
    std::mutex mutex;

    /// Return a pointer to the requested chunk, creating it zero-initialised on first access. This is the only
    /// method which touches _barContents.
    std::byte* getChunk(uint64_t bar, uint64_t chunkIndex);

    void runWriteCallbackFunctionsForAddressRange(AddressRange addressRange);
    std::list<boost::function<void(void)>> findCallbackFunctionsForAddressRange(AddressRange addressRange);

    void setWriteCallbackFunction(AddressRange addressRange, boost::function<void(void)> const& writeCallbackFunction);
    /// returns true if the ranges overlap
    static bool isWriteRangeOverlap(AddressRange firstRange, AddressRange secondRange);

    /// Not write-protected function for internal use only. It does not trigger
    /// the callback function so it can be used inside a callback function for
    /// re-synchronization.
    void writeRegisterWithoutCallback(uint64_t bar, uint64_t address, int32_t data);

    /// register accessors must be friends to access the map and the registers
    template<typename T>
    friend class DummyRegisterAccessor;

    template<typename T>
    friend class DummyMultiplexedRegisterAccessor;

    template<typename RawType>
    friend class DummyRegisterRawAccessor;

    friend class SharedDummyBackend;

    friend class proxies::RawByteAccess;
  };

} // namespace ChimeraTK
