/*
 * DummyRegister.h
 *
 *  Created on: Sep 15, 2015
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_DUMMY_REGISTER_ACCESSOR_H
#define CHIMERA_TK_DUMMY_REGISTER_ACCESSOR_H

#include "DummyBackend.h"
#include "NumericAddressedBackendMuxedRegisterAccessor.h" // for the prefixes to the register names

namespace ChimeraTK {

  /// We are using temporary proxy classes to realise element access with fixed
  /// point conversion. These classes are put into a separate name space, as they
  /// should never be instantiated by the user.
  namespace proxies {
    /*********************************************************************************************************************/
    /// Temporary proxy class for use in the DummyRegister and
    /// DummyMultiplexedRegister classes. Will be returned in place of l.h.s.
    /// references to the register elements, to allow read-write access to registers
    /// governed by a FixedPointConverter.
    template<typename T>
    class DummyRegisterElement {
     public:
      DummyRegisterElement(FixedPointConverter* _fpc, int _nbytes, int32_t* _buffer)
      : fpcptr(_fpc), nbytes(_nbytes), buffer(_buffer) {}

      /// Implicit type conversion to user type T.
      /// This covers already a lot of operations like arithmetics and comparison
      inline operator T() const { return fpcptr->template scalarToCooked<T>(*buffer); }

      /// assignment operator
      inline DummyRegisterElement<T> operator=(T rhs) {
        int32_t raw = fpcptr->toRaw(rhs);
        memcpy(buffer, &raw, nbytes);
        return *this;
      }

      /// pre-increment operator
      inline DummyRegisterElement<T> operator++() {
        T cooked = fpcptr->template scalarToCooked<T>(*buffer);
        return operator=(cooked + 1);
      }

      /// pre-decrement operator
      inline DummyRegisterElement<T> operator--() {
        T cooked = fpcptr->template scalarToCooked<T>(*buffer);
        return operator=(cooked - 1);
      }

      /// post-increment operator
      inline T operator++(int) {
        T cooked = fpcptr->template scalarToCooked<T>(*buffer);
        operator=(cooked + 1);
        return cooked;
      }

      /// post-decrement operator
      inline T operator--(int) {
        T cooked = fpcptr->template scalarToCooked<T>(*buffer);
        operator=(cooked - 1);
        return cooked;
      }

     protected:
      /// constructer when used as a base class in DummyRegister
      DummyRegisterElement() : fpcptr(NULL), nbytes(0), buffer(NULL) {}

      /// fixed point converter to be used for this element
      FixedPointConverter* fpcptr;

      /// number of bytes per word
      int nbytes;

      /// raw buffer of this element
      int32_t* buffer;
    };

    /*********************************************************************************************************************/
    /// Temporary proxy class for sequences, used in the DummyMultiplexedRegister
    /// class. Will be returned by the first [] operator.
    template<typename T>
    class DummyRegisterSequence {
     public:
      DummyRegisterSequence(FixedPointConverter* _fpc, int _nbytes, int _pitch, int32_t* _buffer)
      : fpcptr(_fpc), nbytes(_nbytes), pitch(_pitch), buffer(_buffer) {}

      /// Get or set register content by [] operator.
      inline DummyRegisterElement<T> operator[](unsigned int sample) {
        char* basePtr = reinterpret_cast<char*>(buffer);
        return DummyRegisterElement<T>(fpcptr, nbytes, reinterpret_cast<int32_t*>(basePtr + pitch * sample));
      }

     protected:
      /// fixed point converter to be used for this sequence
      FixedPointConverter* fpcptr;

      /// number of bytes per word
      int nbytes;

      /// pitch in bytes (distance between samples of the same sequence)
      int pitch;

      /// reference to the raw buffer (first word of the sequence)
      int32_t* buffer;

     private:
      /** prevent copying by operator=, since it will be confusing */
      void operator=(const DummyRegisterSequence& rightHandSide) const;
    };
  } // namespace proxies

  /*********************************************************************************************************************/
  /** Class providing a function to check whether a given address is inside the
   * address range of a register or not.
   */
  class DummyRegisterAddressChecker {
   public:
    DummyRegisterAddressChecker(RegisterInfoMap::RegisterInfo _registerInfo) : registerInfo(_registerInfo) {}

    /// check if the given address is in range of the register
    bool isAddressInRange(uint8_t bar, uint32_t address, size_t length) {
      return (bar == registerInfo.bar && address >= registerInfo.address &&
          address + length <= registerInfo.address + registerInfo.nBytes);
    }

   protected:
    /// constructor for derived classes
    DummyRegisterAddressChecker() {}

    /// register map information
    RegisterInfoMap::RegisterInfo registerInfo;
  };

  /*********************************************************************************************************************/
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
  class DummyRegisterAccessor : public proxies::DummyRegisterElement<T>, public DummyRegisterAddressChecker {
   public:
    /// Constructor should normally be called in the constructor of the
    /// DummyBackend implementation. dev must be the pointer to the DummyBackend
    /// to be accessed. A raw pointer is needed, as used inside the DummyBackend
    /// itself. module and name denominate the register entry in the map file.
    DummyRegisterAccessor(DummyBackend* dev, std::string module, std::string name)
    : _dev(dev), fpc(module + "/" + name) {
      _dev->_registerMapping->getRegisterInfo(name, registerInfo, module);
      fpc = FixedPointConverter(
          module + "/" + name, registerInfo.width, registerInfo.nFractionalBits, registerInfo.signedFlag);
      // initialise the base DummyRegisterElement
      proxies::DummyRegisterElement<T>::fpcptr = &fpc;
      proxies::DummyRegisterElement<T>::nbytes = sizeof(int32_t);
      proxies::DummyRegisterElement<T>::buffer = getElement(0);
    }

    /// Get or set register content by [] operator.
    inline proxies::DummyRegisterElement<T> operator[](unsigned int index) { return getProxy(index); }

    /// return number of elements
    unsigned int getNumberOfElements() { return registerInfo.nElements; }

    /// expose = operator from base class
    using proxies::DummyRegisterElement<T>::operator=;

   protected:
    /// pointer to VirtualDevice
    DummyBackend* _dev;

    /// fixed point converter
    FixedPointConverter fpc;

    /// return element
    inline int32_t* getElement(unsigned int index) {
      return &(_dev->_barContents[registerInfo.bar][registerInfo.address / sizeof(int32_t) + index]);
    }

    /// return a proxy object
    inline proxies::DummyRegisterElement<T> getProxy(int index) {
      return proxies::DummyRegisterElement<T>(&fpc, sizeof(int32_t), getElement(index));
    }

   private:
    /** prevent copying by operator=, since it will be confusing */
    void operator=(const DummyRegisterAccessor& rightHandSide) const;
  };

  /*********************************************************************************************************************/
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
  class DummyMultiplexedRegisterAccessor : public DummyRegisterAddressChecker {
   public:
    /// Constructor should normally be called in the constructor of the
    /// DummyBackend implementation. dev must be the pointer to the DummyBackend
    /// to be accessed. A raw pointer is needed, as used inside the DummyBackend
    /// itself. module and name denominate the register entry in the map file.
    /// Note: The string "AREA_MULTIPLEXED_SEQUENCE_" will be prepended to the
    /// name when searching for the register.
    DummyMultiplexedRegisterAccessor(DummyBackend* dev, std::string module, std::string name) : _dev(dev), pitch(0) {
      _dev->_registerMapping->getRegisterInfo(MULTIPLEXED_SEQUENCE_PREFIX + name, registerInfo, module);

      int i = 0;
      while(true) {
        // obtain register information for sequence
        RegisterInfoMap::RegisterInfo elem;
        std::stringstream sequenceNameStream;
        sequenceNameStream << SEQUENCE_PREFIX << name << "_" << i++;
        try {
          _dev->_registerMapping->getRegisterInfo(sequenceNameStream.str(), elem, module);
        }
        catch(ChimeraTK::logic_error&) {
          break;
        }
        // create fixed point converter for sequence
        fpc.push_back(FixedPointConverter(module + "/" + name, elem.width, elem.nFractionalBits, elem.signedFlag));
        // store offsets and number of bytes per word
        offsets.push_back(elem.address);
        nbytes.push_back(elem.nBytes);
        // determine pitch
        pitch += elem.nBytes;
      }

      if(fpc.empty()) {
        throw ChimeraTK::logic_error("No sequenes found for name \"" + name + "\".");
      }

      // compute number of elements per sequence
      nElements = registerInfo.nBytes / pitch;
    }

    /// return number of elements per sequence
    unsigned int getNumberOfElements() { return nElements; }

    /// return number of sequences
    unsigned int getNumberOfSequences() { return fpc.size(); }

    /// Get or set register content by [] operators.
    /// The first [] denotes the sequence (aka. channel number), the second []
    /// indicates the sample inside the sequence. This means that in a sense the
    /// first index is the faster counting index. Example: myMuxRegister[3][987]
    /// will give you the 988th sample of the 4th channel.
    inline proxies::DummyRegisterSequence<T> operator[](unsigned int sequence) {
      int8_t* basePtr = reinterpret_cast<int8_t*>(_dev->_barContents[registerInfo.bar].data());
      int32_t* seq = reinterpret_cast<int32_t*>(basePtr + offsets[sequence]);
      return proxies::DummyRegisterSequence<T>(&(fpc[sequence]), nbytes[sequence], pitch, seq);
    }

   protected:
    /// pointer to VirtualDevice
    DummyBackend* _dev;

    /// pointer to fixed point converter
    std::vector<FixedPointConverter> fpc;

    /// offsets in bytes for sequences
    std::vector<int> offsets;

    /// number of bytes per word for sequences
    std::vector<int> nbytes;

    /// pitch in bytes (distance between samples of the same sequence)
    int pitch;

    /// number of elements per sequence
    unsigned int nElements;

   private:
    /** prevent copying by operator=, since it will be confusing */
    void operator=(const DummyMultiplexedRegisterAccessor& rightHandSide) const;
  };

  /** Accessor for raw 32 bit integer access to the underlying memory space.
   *  Usually you want the interpreted version, but for debugging the converters themselves
   *  and functionality of the NumericAddressedBackendRegisterAccessor we directly
   *  want to write to the registers, without having to mess with absoute addresses
   *  (we still depend on the map file parser, but the whole dummy does. The address
   *   translation is re-done in the dummy, we are not using the one from the regular accessors.)
   */
  class DummyRegisterRawAccessor : public DummyRegisterAddressChecker {
   public:
    /// Implicit type conversion to int32_t.
    /// This basically covers all operators for single integers.
    operator int32_t&() { return *buffer; }

    DummyRegisterRawAccessor(boost::shared_ptr<DeviceBackend> backend, std::string module, std::string name)
    : _backend(boost::dynamic_pointer_cast<DummyBackend>(backend)) {
      assert(_backend);
      _backend->_registerMapping->getRegisterInfo(name, registerInfo, module);
      buffer = &(_backend->_barContents[registerInfo.bar][registerInfo.address / sizeof(int32_t)]);
    }

    DummyRegisterRawAccessor operator=(int32_t rhs) {
      buffer[0] = rhs;
      return *this;
    }

    /// Get or set register content by [] operator.
    int32_t& operator[](unsigned int index) { return buffer[index]; }

    /// return number of elements
    unsigned int getNumberOfElements() { return registerInfo.nElements; }

   protected:
    /// pointer to dummy backend
    boost::shared_ptr<DummyBackend> _backend;

    /// raw buffer of this accessor
    int32_t* buffer;

   private:
    /** prevent copying by operator=, since it will be confusing */
    void operator=(const DummyRegisterRawAccessor& rightHandSide) const;

    // The default copy/move constructor is fine. It will copy the raw pointer to the buffer,
    // together with the shared pointer which holds the corresponding backend with the
    // memory. So it is a consistent copy because the shared pointer is pointing to the
    // same backend instance.
  };

} // namespace ChimeraTK

#endif /* CHIMERA_TK_DUMMY_REGISTER_ACCESSOR_H */
