// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DummyBackend.h"

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
      static std::shared_ptr<RawConverterCapsule<UserType>> makeCapsule(
          const ChimeraTK::NumericAddressedRegisterInfo& info, size_t channelIndex);
    };

    /******************************************************************************************************************/
    /// Temporary proxy class for use in the DummyRegister and
    /// DummyMultiplexedRegister classes. Will be returned in place of l.h.s.
    /// references to the register elements, to allow read-write access to registers
    /// governed by a RawConverter
    template<typename T>
    class DummyRegisterElement {
     public:
      DummyRegisterElement(std::shared_ptr<RawConverterCapsule<T>>& rc, int nbytes, std::byte* buffer)
      : _rc(rc), _nbytes(nbytes), _buffer(buffer) {}

      /// Implicit type conversion to user type T.
      /// This covers already a lot of operations like arithmetic and comparison
      // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
      operator T() const { return _rc->toCooked(_buffer); }

      /// assignment operator
      DummyRegisterElement<T>& operator=(T rhs) {
        _rc->toRaw(rhs, _buffer);
        return *this;
      }

      /// pre-increment operator
      DummyRegisterElement<T> operator++() {
        T cooked = _rc->toCooked(_buffer);
        return operator=(cooked + 1);
      }

      /// pre-decrement operator
      DummyRegisterElement<T> operator--() {
        T cooked = _rc->toCooked(_buffer);

        return operator=(cooked - 1);
      }

      /// post-increment operator
      T operator++(int) {
        T cooked = _rc->toCooked(_buffer);

        operator=(cooked + 1);
        return cooked;
      }

      /// post-decrement operator
      T operator--(int) {
        T cooked = _rc->toCooked(_buffer);

        operator=(cooked - 1);
        return cooked;
      }

     protected:
      /// fixed point converter to be used for this element
      std::shared_ptr<RawConverterCapsule<T>>& _rc;

      /// number of bytes per word
      int _nbytes;

      /// raw buffer of this element
      std::byte* _buffer;
    };

    /******************************************************************************************************************/
    /// Temporary proxy class for sequences, used in the DummyMultiplexedRegister
    /// class. Will be returned by the first [] operator.
    template<typename T>
    class DummyRegisterSequence {
     public:
      DummyRegisterSequence(
          std::shared_ptr<proxies::RawConverterCapsule<T>>& rc, int nbytes, int pitch, int32_t* buffer)
      : _rc(rc), _nbytes(nbytes), _pitch(pitch), _buffer(buffer) {}

      /// Get or set register content by [] operator.
      DummyRegisterElement<T> operator[](unsigned int sample) {
        // todo: Probably ranges are the correct tool in cpp22. In cpp17 this is not available yet. We turn off the
        // warning not to use reinterpret_cast for the time being
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto* basePtr = reinterpret_cast<std::byte*>(_buffer);
        auto* startPtr = basePtr + static_cast<size_t>(_pitch) * sample;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return DummyRegisterElement<T>(_rc, _nbytes, startPtr);
      }

      /// remove assignment operator since it will be confusing */
      void operator=(const DummyRegisterSequence& rightHandSide) const = delete;

     protected:
      /// fixed point converter to be used for this sequence
      std::shared_ptr<proxies::RawConverterCapsule<T>>& _rc;

      /// number of bytes per word
      int _nbytes;

      /// pitch in bytes (distance between samples of the same sequence)
      int _pitch;

      /// reference to the raw buffer (first word of the sequence)
      int32_t* _buffer;
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
    : proxies::DummyRegisterElement<T>(_rc, 0, nullptr), _dev(dev), _path(module + "/" + name),
      _registerInfo(_dev->_registerMap.getBackendRegister(_path)),
      _rc(proxies::RawConverterCapsule<T>::makeCapsule(_registerInfo, 0)) {
      // initialise the base DummyRegisterElement
      proxies::DummyRegisterElement<T>::_nbytes = sizeof(int32_t);
      proxies::DummyRegisterElement<T>::_buffer = getElement(0);
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
          {static_cast<uint8_t>(_registerInfo.bar), static_cast<uint32_t>(_registerInfo.address),
              _registerInfo.nElements * _registerInfo.elementPitchBits / 8},
          writeCallback);
    }

    const NumericAddressedRegisterInfo& getRegisterInfo() { return _registerInfo; }

    /** Get a lock to safely modify the buffer in a multi-treaded environment. You have to release it as soon as
     * possible because it will block all other functionality of the Dummy and all application threads which use it.
     */
    std::unique_lock<std::mutex> getBufferLock() { return std::unique_lock<std::mutex>(_dev->mutex); }

   protected:
    /// pointer to VirtualDevice
    DummyBackend* _dev;

    /// path of the register
    RegisterPath _path;

    /// register map information
    NumericAddressedRegisterInfo _registerInfo;

    /// fixed point converter
    std::shared_ptr<proxies::RawConverterCapsule<T>> _rc;

    /// return element
    std::byte* getElement(unsigned int index) {
      auto* ptr = &(_dev->_barContents[_registerInfo.bar][_registerInfo.address / sizeof(int32_t) + index]);
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      return reinterpret_cast<std::byte*>(ptr);
    }

    /// return a proxy object
    proxies::DummyRegisterElement<T> getProxy(int index) {
      return proxies::DummyRegisterElement<T>(_rc, sizeof(int32_t), getElement(index));
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
        // store offsets and number of bytes per word
        assert(c.bitOffset % 8 == 0);
        _offsets.push_back(_registerInfo.address + c.bitOffset / 8);
        _nbytes.push_back((c.width - 1) / 8 + 1); // width/8 rounded up
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
      // todo: Probably ranges are the correct tool in cpp22. In cpp17 this is not available yet. We turn off the
      // warning not to use reinterpret_cast for the time being
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      auto* basePtr = reinterpret_cast<std::byte*>(_dev->_barContents[_registerInfo.bar].data());
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      auto* seq = reinterpret_cast<int32_t*>(basePtr + _offsets[sequence]);
      return proxies::DummyRegisterSequence<T>(_rc[sequence], _nbytes[sequence], _pitch, seq);
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

    /// pointer to fixed point converter
    std::vector<std::shared_ptr<proxies::RawConverterCapsule<T>>> _rc;

    /// offsets in bytes for sequences
    std::vector<uint32_t> _offsets;

    /// number of bytes per word for sequences
    std::vector<uint32_t> _nbytes;

    /// pitch in bytes (distance between samples of the same sequence)
    int _pitch = {0};

    /// number of elements per sequence
    unsigned int _nElements;
  };

  /** Accessor for raw 32 bit integer access to the underlying memory space.
   *  Usually you want the interpreted version, but for debugging the converters themselves
   *  and functionality of the NumericAddressedBackendRegisterAccessor we directly
   *  want to write to the registers, without having to mess with absolute addresses
   *  (we still depend on the map file parser, but the whole dummy does. The address
   *   translation is re-done in the dummy, we are not using the one from the regular accessors.)
   *
   *   WARNING: You must not touch any data content of the accessor without holding a lock to
   *   the memory mutex for the internal data buffer (see getBufferLock()).
   */
  class DummyRegisterRawAccessor {
   public:
    /// Implicit type conversion to int32_t.
    /// This basically covers all operators for single integers.
    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    operator int32_t&() { return *_buffer; }

    DummyRegisterRawAccessor(
        boost::shared_ptr<DeviceBackend> const& backend, std::string const& module, std::string const& name)
    : _backend(boost::dynamic_pointer_cast<DummyBackend>(backend)) {
      assert(_backend);
      _registerInfo = _backend->_registerMap.getBackendRegister(module + "." + name);
      _buffer = &(_backend->_barContents[_registerInfo.bar][_registerInfo.address / sizeof(int32_t)]);
    }

    // declare that we want the default copy constructor. Needed because we have a custom = operator
    // The default copy/move constructor is fine. It will copy the raw pointer to the buffer,
    // together with the shared pointer which holds the corresponding backend with the
    // memory. So it is a consistent copy because the shared pointer is pointing to the
    // same backend instance.
    DummyRegisterRawAccessor(const DummyRegisterRawAccessor&) = default;

    /// remove assignment operator since it will be confusing
    void operator=(const DummyRegisterRawAccessor& rightHandSide) const = delete;

    DummyRegisterRawAccessor& operator=(int32_t rhs) {
      _buffer[0] = rhs;
      return *this;
    }

    /// Get or set register content by [] operator.
    int32_t& operator[](unsigned int index) { return _buffer[index]; }

    /// return number of elements
    [[nodiscard]] unsigned int getNumberOfElements() const { return _registerInfo.nElements; }

    /** Get a lock to safely modify the buffer. You have to release it as soon as possible because it will block all
     * other functionality of the Dummy. This is a really low low level debugging interface!
     */
    std::unique_lock<std::mutex> getBufferLock() { return std::unique_lock<std::mutex>(_backend->mutex); }

   protected:
    /// pointer to dummy backend
    boost::shared_ptr<DummyBackend> _backend;

    /// register map information
    NumericAddressedRegisterInfo _registerInfo;

    /// raw buffer of this accessor
    int32_t* _buffer;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
