// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DummyBackend.h"

#include <cstddef>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

namespace ChimeraTK {

  /// We are using temporary proxy classes to realise element access with fixed
  /// point conversion. These classes are put into a separate name space, as they
  /// should never be instantiated by the user.
  namespace proxies {

    /******************************************************************************************************************/

    /**
     * This abstract class encapsulates a RawConverter to erase the exact RawConverter type (with all its template
     * parameters).
     */
    template<typename UserType>
    class RawConverterCapsule {
     public:
      virtual ~RawConverterCapsule() = default;
      virtual UserType toCooked(std::byte* raw) = 0;
      virtual void toRaw(UserType cooked, std::byte* raw) = 0;

      /// Size in bytes of the raw element this capsule reads/writes (the size of its RawType, which equals the
      /// element width). A user type may be narrower than one storage element (e.g. a single-bit flag stored in a
      /// 32-bit word), so the caller must not derive the access size from the user type's width.
      [[nodiscard]] virtual size_t rawSize() const = 0;

      static std::shared_ptr<RawConverterCapsule<UserType>> makeCapsule(
          const ChimeraTK::NumericAddressedRegisterInfo& info, size_t channelIndex);
    };

    /******************************************************************************************************************/

    /// Value-semantic access to a single element's bytes through the chunked storage of a DummyBackend. It never
    /// exposes a pointer into the internal buffers; each access copies exactly the requested number of bytes via the
    /// chunked transfer helper. It acquires the backend's storage lock for the short duration of the access.
    class RawByteAccess {
     public:
      RawByteAccess(DummyBackend* dev, uint64_t bar, uint64_t byteOffset)
      : _dev(dev), _bar(bar), _byteOffset(byteOffset) {}

      void read(std::byte* destination, size_t nbytes) const {
        std::lock_guard<std::mutex> lock(_dev->mutex);
        DummyBackendBase::transferChunked(
            _bar, _byteOffset, nbytes,
            [this](uint64_t bar, uint64_t chunkIndex) { return _dev->getChunk(bar, chunkIndex); },
            [destination](size_t size, const std::byte* chunk, size_t bytesDone) {
              std::memcpy(destination + bytesDone, chunk, size);
            });
      }

      void write(const std::byte* source, size_t nbytes) const {
        std::lock_guard<std::mutex> lock(_dev->mutex);
        DummyBackendBase::transferChunked(
            _bar, _byteOffset, nbytes,
            [this](uint64_t bar, uint64_t chunkIndex) { return _dev->getChunk(bar, chunkIndex); },
            [source](
                size_t size, std::byte* chunk, size_t bytesDone) { std::memcpy(chunk, source + bytesDone, size); });
      }

     private:
      DummyBackend* _dev;
      uint64_t _bar;
      uint64_t _byteOffset;
    };

    /******************************************************************************************************************/
    /// Temporary proxy class for use in the DummyRegister and
    /// DummyMultiplexedRegister classes. Will be returned in place of l.h.s.
    /// references to the register elements, to allow read-write access to registers
    /// governed by a RawConverter. It stores the element's byte access instead of a raw pointer, so it works across
    /// chunk boundaries and for arbitrarily byte-aligned registers.
    template<typename T>
    class DummyRegisterElement {
     public:
      DummyRegisterElement(std::shared_ptr<RawConverterCapsule<T>> rc, uint64_t nbytes, RawByteAccess access)
      : _rc(std::move(rc)), _nbytes(nbytes), _access(access) {}

      /// Implicit type conversion to user type T.
      /// This covers already a lot of operations like arithmetic and comparison
      // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
      operator T() const {
        std::vector<std::byte> raw(_nbytes);
        _access.read(raw.data(), _nbytes);
        return _rc->toCooked(raw.data());
      }

      /// assignment operator
      DummyRegisterElement<T>& operator=(T rhs) {
        std::vector<std::byte> raw(_nbytes);
        _rc->toRaw(rhs, raw.data());
        _access.write(raw.data(), _nbytes);
        return *this;
      }

      /// pre-increment operator
      DummyRegisterElement<T> operator++() {
        T cooked = T(*this);
        return operator=(cooked + 1);
      }

      /// pre-decrement operator
      DummyRegisterElement<T> operator--() {
        T cooked = T(*this);
        return operator=(cooked - 1);
      }

      /// post-increment operator
      T operator++(int) {
        T cooked = T(*this);
        operator=(cooked + 1);
        return cooked;
      }

      /// post-decrement operator
      T operator--(int) {
        T cooked = T(*this);
        operator=(cooked - 1);
        return cooked;
      }

     protected:
      /// fixed point converter to be used for this element
      std::shared_ptr<RawConverterCapsule<T>> _rc;

      /// number of bytes per element
      uint64_t _nbytes;

      /// value-semantic access to the element's bytes
      RawByteAccess _access;
    };

    /******************************************************************************************************************/
    /// Temporary proxy class for sequences, used in the DummyMultiplexedRegister
    /// class. Will be returned by the first [] operator.
    template<typename T>
    class DummyRegisterSequence {
     public:
      DummyRegisterSequence(std::shared_ptr<RawConverterCapsule<T>> rc, uint64_t nbytes, uint64_t pitch,
          DummyBackend* dev, uint64_t bar, uint64_t byteOffset)
      : _rc(std::move(rc)), _nbytes(nbytes), _pitch(pitch), _dev(dev), _bar(bar), _byteOffset(byteOffset) {}

      /// Get or set register content by [] operator.
      DummyRegisterElement<T> operator[](unsigned int sample) {
        return DummyRegisterElement<T>(_rc, _nbytes, RawByteAccess(_dev, _bar, _byteOffset + _pitch * sample));
      }

      /// remove assignment operator since it will be confusing */
      void operator=(const DummyRegisterSequence& rightHandSide) const = delete;

     protected:
      /// fixed point converter to be used for this sequence
      std::shared_ptr<RawConverterCapsule<T>> _rc;

      /// number of bytes per element
      uint64_t _nbytes;

      /// pitch in bytes (distance between samples of the same sequence)
      uint64_t _pitch;

      /// backend holding the chunked storage
      DummyBackend* _dev;

      /// bar and byte offset of the first element of the sequence
      uint64_t _bar;
      uint64_t _byteOffset;
    };

    /******************************************************************************************************************/

  } // namespace proxies

  /********************************************************************************************************************/
  /** Register accessor for accessing single word or 1D array registers internally
   * of a DummyBackend implementation. This accessor should be used to access the
   * dummy registers through the "backdoor" when unit-testing e.g. a library or
   * when implementing a device in the VirtualLab framework. A simple access is
   * provided through the operators and implicit type conversions. The [] operator
   * will return a temporary proxy class, which deals with converting read and
   * write operations of a single word of the register. The temporary proxy
   * implements all needed operators and the implicit type conversion to the type
   * T, so it can be used as it were a variable of the type T in most places.
   *
   *  This class inherits from the DummyRegisterElement proxy, to avoid
   * reimplementing the same interface again. It is thus possible to access
   * through the DummyRegister class directly the first element of the register,
   * without using the [0] operator.
   */
  template<typename T>
  class DummyRegisterAccessor : public proxies::DummyRegisterElement<T> {
   public:
    /// Constructor should normally be called in the constructor of the
    /// DummyBackend implementation. dev must be the pointer to the DummyBackend
    /// to be accessed. A raw pointer is needed, as used inside the DummyBackend
    /// itself. module and name denominate the register entry in the map file.
    DummyRegisterAccessor(DummyBackend* dev, std::string const& module, std::string const& name)
    : proxies::DummyRegisterElement<T>(nullptr, 0, proxies::RawByteAccess(dev, 0, 0)), _dev(dev),
      _path(module + "/" + name), _registerInfo(_dev->_registerMap.getBackendRegister(_path)) {
      // initialise the base DummyRegisterElement; the base owns the converter capsule and the byte access, so the
      // direct element access through the inherited operators works as well
      this->_rc = proxies::RawConverterCapsule<T>::makeCapsule(_registerInfo, 0);
      this->_nbytes = this->_rc->rawSize();
      this->_access = proxies::RawByteAccess(_dev, _registerInfo.bar, _registerInfo.address);
    }
    // declare that we want the default copy constructor. Needed because we have a custom = operator
    DummyRegisterAccessor(const DummyRegisterAccessor&) = default;

    /// remove assignment operator since it will be confusing */
    void operator=(const DummyRegisterAccessor& rightHandSide) const = delete;

    /// Get or set register content by [] operator.
    proxies::DummyRegisterElement<T> operator[](unsigned int index) { return getProxy(index); }

    /// return number of elements
    unsigned int getNumberOfElements() { return _registerInfo.nElements; }

    /// expose = operator from base class
    using proxies::DummyRegisterElement<T>::operator=;

    /// Return the backend
    [[nodiscard]] DummyBackend& getBackend() const { return *_dev; }

    /// Return the register path
    [[nodiscard]] const RegisterPath& getRegisterPath() const { return _path; }

    /// Set callback function which is called when the register is written to (through the normal Device interface)
    void setWriteCallback(const std::function<void()>& writeCallback) {
      assert(_registerInfo.elementPitchBits % 8 == 0);
      _dev->setWriteCallbackFunction(
          {_registerInfo.bar, _registerInfo.address,
              static_cast<uint64_t>(_registerInfo.nElements) * _registerInfo.elementPitchBits / 8},
          writeCallback);
    }

    const NumericAddressedRegisterInfo& getRegisterInfo() { return _registerInfo; }

   protected:
    /// pointer to VirtualDevice
    DummyBackend* _dev;

    /// path of the register
    RegisterPath _path;

    /// register map information
    NumericAddressedRegisterInfo _registerInfo;

    /// return a proxy object
    proxies::DummyRegisterElement<T> getProxy(unsigned int index) {
      assert(_registerInfo.elementPitchBits % 8 == 0);
      return proxies::DummyRegisterElement<T>(this->_rc, this->_rc->rawSize(),
          proxies::RawByteAccess(_dev, _registerInfo.bar,
              _registerInfo.address + static_cast<uint64_t>(index) * (_registerInfo.elementPitchBits / 8)));
    }
  };

  /********************************************************************************************************************/
  /** Register accessor for accessing multiplexed 2D array registers internally of
   * a DummyBackend implementation. This accessor is similar to the DummyRegister
   * accessor but works with multiplexed registers. The interface is similar to
   * the DummyRegister, it just introduces a second level of the [] operator. The
   * first [] operator takes the sequence number (aka. channel number) as an
   * argument, the second [] operator needs the index (aka. sample numeber) inside
   * the sequence. Returned is then again a temporary proxy element of the same
   * type as for the DummyRegister accessor, so it can again be used mostly like a
   * variable of the type T.
   */
  template<typename T>
  class DummyMultiplexedRegisterAccessor {
   public:
    /// Constructor should normally be called in the constructor of the
    /// DummyBackend implementation. dev must be the pointer to the DummyBackend
    /// to be accessed. A raw pointer is needed, as used inside the DummyBackend
    /// itself. module and name denominate the register entry in the map file.
    /// Note: The string "AREA_MULTIPLEXED_SEQUENCE_" will be prepended to the
    /// name when searching for the register.
    DummyMultiplexedRegisterAccessor(DummyBackend* dev, std::string const& module, std::string const& name)
    : _dev(dev), _path(module + "/" + name) {
      _registerInfo = _dev->_registerMap.getBackendRegister(module + "." + name);

      // create fixed point converters for each channel
      size_t ci = 0;
      for(auto& c : _registerInfo.channels) {
        // create fixed point converter for sequence
        _rc.emplace_back(proxies::RawConverterCapsule<T>::makeCapsule(_registerInfo, ci));
        // store offsets and number of bytes per element
        assert(c.bitOffset % 8 == 0);
        _offsets.push_back(_registerInfo.address + c.bitOffset / 8);
        _nbytes.push_back(_rc.back()->rawSize()); // size of the channel's raw element, which the capsule reads/writes
        ++ci;
      }

      if(_rc.empty()) {
        throw ChimeraTK::logic_error("No sequences found for name \"" + name + "\".");
      }

      // cache some information
      _nElements = _registerInfo.nElements;
      assert(_registerInfo.elementPitchBits % 8 == 0);
      _pitch = _registerInfo.elementPitchBits / 8;
    }

    /// remove assignment operator since it will be confusing
    void operator=(const DummyMultiplexedRegisterAccessor& rightHandSide) const = delete;

    // declare that we want the default copy constructor. Needed because we have a custom = operator
    DummyMultiplexedRegisterAccessor(const DummyMultiplexedRegisterAccessor&) = default;

    /// return number of elements per sequence
    unsigned int getNumberOfElements() { return _nElements; }

    /// return number of sequences
    unsigned int getNumberOfSequences() { return _rc.size(); }

    /// Get or set register content by [] operators.
    /// The first [] denotes the sequence (aka. channel number), the second []
    /// indicates the sample inside the sequence. This means that in a sense the
    /// first index is the faster counting index. Example: myMuxRegister[3][987]
    /// will give you the 988th sample of the 4th channel.
    proxies::DummyRegisterSequence<T> operator[](unsigned int sequence) {
      return proxies::DummyRegisterSequence<T>(
          _rc[sequence], _nbytes[sequence], _pitch, _dev, _registerInfo.bar, _offsets[sequence]);
    }

    /// Return the backend
    [[nodiscard]] DummyBackend& getBackend() const { return *_dev; }

    /// Return the register path
    [[nodiscard]] const RegisterPath& getRegisterPath() const { return _path; }

    [[nodiscard]] const NumericAddressedRegisterInfo& getRegisterInfo() { return _registerInfo; }

   protected:
    /// pointer to VirtualDevice
    DummyBackend* _dev;

    /// register map information
    NumericAddressedRegisterInfo _registerInfo;

    /// path of the register
    RegisterPath _path;

    /// fixed point converter for each sequence
    std::vector<std::shared_ptr<proxies::RawConverterCapsule<T>>> _rc;

    /// offsets in bytes for sequences
    std::vector<uint64_t> _offsets;

    /// number of bytes per element for sequences
    std::vector<uint64_t> _nbytes;

    /// pitch in bytes (distance between samples of the same sequence)
    uint64_t _pitch = {0};

    /// number of elements per sequence
    unsigned int _nElements;
  };

  /********************************************************************************************************************/
  /// Proxy for the DummyRegisterRawAccessor. It provides value semantics: it converts to and from the raw type and
  /// implements the compound assignment operators, inc/dec and the binary operators & and ~. Every operation reads or
  /// writes the memory directly, so existing expressions keep working.
  template<typename RawType>
  class DummyRegisterRawElement {
   public:
    DummyRegisterRawElement(boost::shared_ptr<DummyBackend> backend, uint64_t bar, uint64_t byteOffset)
    : _backend(std::move(backend)), _bar(bar), _byteOffset(byteOffset) {}

    /// Implicit type conversion to RawType.
    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    operator RawType() const { return get(); }

    [[nodiscard]] RawType get() const {
      RawType value{};
      auto* bytes = reinterpret_cast<std::byte*>(&value); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
      proxies::RawByteAccess(_backend.get(), _bar, _byteOffset).read(bytes, sizeof(RawType));
      return value;
    }

    void set(RawType value) {
      const auto* bytes =
          reinterpret_cast<const std::byte*>(&value); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
      proxies::RawByteAccess(_backend.get(), _bar, _byteOffset).write(bytes, sizeof(RawType));
    }

    DummyRegisterRawElement& operator=(RawType rhs) {
      set(rhs);
      return *this;
    }

    DummyRegisterRawElement& operator+=(RawType rhs) { return operator=(get() + rhs); }
    DummyRegisterRawElement& operator-=(RawType rhs) { return operator=(get() - rhs); }
    DummyRegisterRawElement& operator*=(RawType rhs) { return operator=(get() * rhs); }
    DummyRegisterRawElement& operator/=(RawType rhs) { return operator=(get() / rhs); }
    DummyRegisterRawElement& operator%=(RawType rhs) { return operator=(get() % rhs); }
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    DummyRegisterRawElement& operator&=(RawType rhs) { return operator=(get() & rhs); }
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    DummyRegisterRawElement& operator|=(RawType rhs) { return operator=(get() | rhs); }
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    DummyRegisterRawElement& operator^=(RawType rhs) { return operator=(get() ^ rhs); }
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    DummyRegisterRawElement& operator<<=(RawType rhs) { return operator=(get() << rhs); }
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    DummyRegisterRawElement& operator>>=(RawType rhs) { return operator=(get() >> rhs); }

    DummyRegisterRawElement& operator++() { return operator=(get() + 1); }
    DummyRegisterRawElement& operator--() { return operator=(get() - 1); }

    RawType operator++(int) {
      RawType value = get();
      set(value + 1);
      return value;
    }

    RawType operator--(int) {
      RawType value = get();
      set(value - 1);
      return value;
    }

    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    RawType operator&(RawType rhs) const { return get() & rhs; }
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    RawType operator~() const { return ~get(); }

   protected:
    /// pointer to dummy backend
    boost::shared_ptr<DummyBackend> _backend;

    /// bar and byte offset of the raw element
    uint64_t _bar;
    uint64_t _byteOffset;
  };

  /********************************************************************************************************************/
  /** Accessor for raw integer access to the underlying memory space.
   *  Usually you want the interpreted version, but for debugging the converters themselves
   *  and functionality of the NumericAddressedBackendRegisterAccessor we directly
   *  want to write to the registers, without having to mess with absolute addresses
   *  (we still depend on the map file parser, but the whole dummy does. The address
   *   translation is re-done in the dummy, we are not using the one from the regular accessors.)
   *
   *   The accessor returns value-semantic proxies. Every element access acquires the backend's storage lock for its
   *   short duration, so the caller does not need to hold any lock.
   */
  template<typename RawType = int32_t>
  class DummyRegisterRawAccessor : public DummyRegisterRawElement<RawType> {
   public:
    static_assert(std::is_integral_v<RawType>, "DummyRegisterRawAccessor requires an integral RawType.");

    DummyRegisterRawAccessor(
        boost::shared_ptr<DeviceBackend> const& backend, std::string const& module, std::string const& name)
    : DummyRegisterRawElement<RawType>(boost::dynamic_pointer_cast<DummyBackend>(backend), 0, 0) {
      assert(this->_backend);
      _registerInfo = this->_backend->_registerMap.getBackendRegister(module + "." + name);
      assert(static_cast<uint64_t>(_registerInfo.elementPitchBits) / 8 == sizeof(RawType));
      this->_bar = _registerInfo.bar;
      this->_byteOffset = _registerInfo.address;
    }

    /// Register-less construction for a whole bar. It is used by the deprecated bar-contents compatibility shim,
    /// because a whole bar has no register map entry. The bar's word count is derived from the map file, but any
    /// address is accessible even beyond it.
    DummyRegisterRawAccessor(boost::shared_ptr<DummyBackend> const& dev, uint64_t bar)
    : DummyRegisterRawElement<RawType>(dev, bar, 0) {
      assert(this->_backend);
      _registerInfo.bar = bar;
      _registerInfo.address = 0;
      _registerInfo.nElements = static_cast<uint32_t>(this->_backend->getBarSizeInBytes(bar) / sizeof(RawType));
      _registerInfo.elementPitchBits = sizeof(RawType) * 8U;
      _registerInfo.channels.emplace_back();
    }

    // declare that we want the default copy constructor. Needed because we have a custom = operator
    DummyRegisterRawAccessor(const DummyRegisterRawAccessor&) = default;

    /// remove assignment operator since it will be confusing
    void operator=(const DummyRegisterRawAccessor& rightHandSide) const = delete;

    using DummyRegisterRawElement<RawType>::operator=;

    /// Get or set register content by [] operator.
    DummyRegisterRawElement<RawType> operator[](unsigned int index) {
      assert(_registerInfo.elementPitchBits % 8 == 0);
      return DummyRegisterRawElement<RawType>(this->_backend, _registerInfo.bar,
          _registerInfo.address + static_cast<uint64_t>(index) * (_registerInfo.elementPitchBits / 8));
    }

    /// return number of elements
    [[nodiscard]] unsigned int getNumberOfElements() const { return _registerInfo.nElements; }

   protected:
    /// register map information
    NumericAddressedRegisterInfo _registerInfo;
  };

  /// Deduction guide: allows `DummyRegisterRawAccessor acc{...}` without an explicit template argument.
  DummyRegisterRawAccessor(boost::shared_ptr<DeviceBackend> const&, std::string const&, std::string const&)
      -> DummyRegisterRawAccessor<int32_t>;

  /********************************************************************************************************************/

  /// Definition of the deprecated bar-contents compatibility shim. It is out of line, after DummyRegisterRawAccessor is
  /// complete, because DummyBackend's class definition cannot use the accessor type (it would be incomplete there).
  inline auto DummyBackend::getWholeBarRawAccessor(uint64_t bar) {
    // Non-owning shared_ptr: the accessor is used immediately and BarContentsCompat's backend outlives it, so there is
    // no need to share ownership (and shared_from_this() would be const here).
    return DummyRegisterRawAccessor<int32_t>(boost::shared_ptr<DummyBackend>(this, [](auto*) {}), bar);
  }

  inline auto DummyBackend::BarContentsCompat::operator[](uint64_t bar) const {
    return _backend->getWholeBarRawAccessor(bar);
  }

} // namespace ChimeraTK
