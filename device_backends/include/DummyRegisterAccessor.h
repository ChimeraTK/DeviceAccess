/*
 * DummyRegister.h
 *
 *  Created on: Sep 15, 2015
 *      Author: Martin Hierholzer
 */

#ifndef SOURCE_DIRECTORY__INCLUDE_DUMMYREGISTER_H_
#define SOURCE_DIRECTORY__INCLUDE_DUMMYREGISTER_H_

#include "DummyBackend.h"
#include "MultiplexedDataAccessor.h"    // for the prefixes to the register names

namespace mtca4u {

  /// Exception class
  class DummyRegisterException : public Exception {
    public:
      enum {EMPTY_AREA};
      DummyRegisterException(const std::string &message, unsigned int exceptionID)
      : Exception( message, exceptionID ){}
  };

  /// We are using temporary proxy classes to realise element access with fixed point conversion. These classes
  /// are put into a separate name space, as they should never be instantiated by the user.
  namespace proxies {
    /*********************************************************************************************************************/
    /// Temporary proxy class for use in the DummyRegister and DummyMultiplexedRegister classes.
    /// Will be returned in place of l.h.s. references to the register elements, to allow read-write access to
    /// registers governed by a FixedPointConverter.
    template<typename T>
    class DummyRegisterElement {

      public:

        DummyRegisterElement(FixedPointConverter *_fpc, int _nbytes, uint32_t *_buffer)
      : fpcptr(_fpc), nbytes(_nbytes), buffer(_buffer) {}

        /// Implicit type conversion to user type T.
        /// This covers already a lot of operations like arithmetics and comparison
        inline operator T() {
          return fpcptr->template toCooked<T>(*buffer);
        }

        /// assignment operator
        inline DummyRegisterElement<T> operator=(T rhs)
        {
          uint32_t raw = fpcptr->toRaw(rhs);
          memcpy(buffer, &raw, nbytes);
          return *this;
        }

        /// pre-increment operator
        inline DummyRegisterElement<T> operator++() {
          T v = fpcptr->template toCooked<T>(*buffer);
          return operator=( v + 1 );
        }

        /// pre-decrement operator
        inline DummyRegisterElement<T> operator--() {
          T v = fpcptr->template toCooked<T>(*buffer);
          return operator=( v - 1 );
        }

        /// post-increment operator
        inline T operator++(int) {
          T v = fpcptr->template toCooked<T>(*buffer);
          operator=( v + 1 );
          return v;
        }

        /// post-decrement operator
        inline T operator--(int) {
          T v = fpcptr->template toCooked<T>(*buffer);
          operator=( v - 1 );
          return v;
        }

      protected:

        /// constructer when used as a base class in DummyRegister
        DummyRegisterElement() : fpcptr(NULL),nbytes(0),buffer(NULL) {}

        /// fixed point converter to be used for this element
        FixedPointConverter *fpcptr;

        /// number of bytes per word
        int nbytes;

        /// raw buffer of this element
        uint32_t *buffer;
    };

    /*********************************************************************************************************************/
    /// Temporary proxy class for sequences, used in the DummyMultiplexedRegister class.
    /// Will be returned by the first [] operator.
    template<typename T>
    class DummyRegisterSequence {

      public:

        DummyRegisterSequence(FixedPointConverter *_fpc, int _nbytes, int _pitch, uint32_t *_buffer)
      : fpcptr(_fpc), nbytes(_nbytes), pitch(_pitch), buffer(_buffer) {}

        /// Get or set register content by [] operator.
        inline DummyRegisterElement<T> operator[](unsigned int sample)
        {
          char *basePtr = reinterpret_cast<char*>(buffer);
          return DummyRegisterElement<T>(fpcptr, nbytes, reinterpret_cast<uint32_t*>(basePtr + pitch*sample) );
        }

      protected:

        /// fixed point converter to be used for this sequence
        FixedPointConverter *fpcptr;

        /// number of bytes per word
        int nbytes;

        /// pitch in bytes (distance between samples of the same sequence)
        int pitch;

        /// reference to the raw buffer (first word of the sequence)
        uint32_t *buffer;
    };
  }     // namespace proxies


  /*********************************************************************************************************************/
  /** Register accessor for accessing single word or 1D array registers internally of a DummyBackend implementation.
   *  This accessor should be used to access the dummy registers through the "backdoor" when unit-testing e.g. a library
   *  or when implementing a device in the VirtualLab framework. A simple access is provided through the operators and
   *  implicit type conversions. The [] operator will return a temporary proxy class, which deals with converting read
   *  and write operations of a single word of the register. The temporary proxy implements all needed operators and
   *  the implicit type conversion to the type T, so it can be used as it were a variable of the type T in most places.
   *
   *  This class inherits from the DummyRegisterElement proxy, to avoid reimplementing the same interface again. It is
   *  thus possible to access through the DummyRegister class directly the first element of the register, without
   *  using the [0] operator.
   */
  template<typename T>
  class DummyRegisterAccessor : public proxies::DummyRegisterElement<T> {
    public:

      /// Constructor should normally be called in the constructor of the DummyBackend implementation.
      /// dev must be the pointer to the DummyBackend to be accessed. A raw pointer is needed, as used inside the
      /// DummyBackend itself. module and name denominate the register entry in the map file.
      DummyRegisterAccessor(DummyBackend *dev, std::string module, std::string name)
    : _dev(dev)
    {
        _dev->_registerMapping->getRegisterInfo(name, registerInfo, module);
        fpc =  FixedPointConverter(registerInfo.reg_width, registerInfo.reg_frac_bits, registerInfo.reg_signed);
        // initialise the base DummyRegisterElement
        proxies::DummyRegisterElement<T>::fpcptr = &fpc;
        proxies::DummyRegisterElement<T>::nbytes = sizeof(uint32_t);
        proxies::DummyRegisterElement<T>::buffer = getElement(0);
    }

      /// Get or set register content by [] operator.
      inline proxies::DummyRegisterElement<T> operator[](unsigned int index)
      {
        return getProxy(index);
      }

      /// return number of elements
      unsigned int getNumberOfElements()
      {
        return registerInfo.reg_elem_nr;
      }

      /// expose = operator from base class
      using proxies::DummyRegisterElement<T>::operator=;

    protected:

      /// register map information
      RegisterInfoMap::RegisterInfo registerInfo;

      /// pointer to VirtualDevice
      DummyBackend *_dev;

      /// fixed point converter
      FixedPointConverter fpc;

      /// return element
      inline uint32_t* getElement(unsigned int index) {
        return reinterpret_cast<uint32_t*>
        (&(_dev->_barContents[registerInfo.reg_bar][registerInfo.reg_address/sizeof(uint32_t) + index]));
      }

      /// return a proxy object
      inline proxies::DummyRegisterElement<T> getProxy(int index) {
        return proxies::DummyRegisterElement<T>(&fpc, sizeof(uint32_t), getElement(index));
      }

  };

  /*********************************************************************************************************************/
  /** Register accessor for accessing multiplexed 2D array registers internally of a DummyBackend implementation.
   *  This accessor is similar to the DummyRegister accessor but works with multiplexed registers. The interface is
   *  similar to the DummyRegister, it just introduces a second level of the [] operator. The first [] operator takes
   *  the sequence number (aka. channel number) as an argument, the second [] operator needs the index (aka. sample
   *  numeber) inside the sequence. Returned is then again a temporary proxy element of the same type as for the
   *  DummyRegister accessor, so it can again be used mostly like a variable of the type T.
   */
  template<typename T>
  class DummyMultiplexedRegisterAccessor {
    public:

      /// Constructor should normally be called in the constructor of the DummyBackend implementation.
      /// dev must be the pointer to the DummyBackend to be accessed. A raw pointer is needed, as used inside the
      /// DummyBackend itself. module and name denominate the register entry in the map file.
      /// Note: The string "AREA_MULTIPLEXED_SEQUENCE_" will be prepended to the name when searching for the register.
      DummyMultiplexedRegisterAccessor(DummyBackend *dev, std::string module, std::string name)
    : _dev(dev), pitch(0)
    {
        _dev->_registerMapping->getRegisterInfo(MULTIPLEXED_SEQUENCE_PREFIX+name, registerInfo, module);

        int i = 0;
        while(true) {
          // obtain register information for sequence
          RegisterInfoMap::RegisterInfo elem;
          std::stringstream sequenceNameStream;
          sequenceNameStream << SEQUENCE_PREFIX << name << "_" << i++;
          try{
            _dev->_registerMapping->getRegisterInfo( sequenceNameStream.str(), elem, module );
          }
          catch(MapFileException &) {
            break;
          }
          // create fixed point converter for sequence
          fpc.push_back( FixedPointConverter(elem.reg_width, elem.reg_frac_bits, elem.reg_signed) );
          // store offsets and number of bytes per word
          offsets.push_back(elem.reg_address);
          nbytes.push_back(elem.reg_size);
          // determine pitch
          pitch += elem.reg_size;
        }

        if(fpc.empty()) {
          throw DummyRegisterException("No sequenes found for name \""+name+"\".",DummyRegisterException::EMPTY_AREA);
        }

        // compute number of elements per sequence
        nElements = registerInfo.reg_elem_nr/fpc.size();
    }

      /// return number of elements per sequence
      unsigned int getNumberOfElements()
      {
        return nElements;
      }

      /// return number of sequences
      unsigned int getNumberOfSequences()
      {
        return fpc.size();
      }

      /// Get or set register content by [] operators.
      /// The first [] denotes the sequence (aka. channel number), the second [] indicates the sample inside the sequence.
      /// This means that in a sense the first index is the faster counting index.
      /// Example: myMuxRegister[3][987] will give you the 988th sample of the 4th channel.
      inline proxies::DummyRegisterSequence<T> operator[](unsigned int sequence)
      {
        int8_t *basePtr = reinterpret_cast<int8_t*>(_dev->_barContents[registerInfo.reg_bar].data());
        uint32_t *seq = reinterpret_cast<uint32_t*>(basePtr + (registerInfo.reg_address + offsets[sequence]));
        return proxies::DummyRegisterSequence<T>(&(fpc[sequence]), nbytes[sequence], pitch, seq);
      }

    protected:

      /// register map information
      RegisterInfoMap::RegisterInfo registerInfo;

      /// pointer to VirtualDevice
      DummyBackend *_dev;

      /// pointer to fixed point converter
      std::vector<FixedPointConverter> fpc;

      /// offsets in bytes for sequences
      std::vector<int> offsets;

      /// number of bytes per word for sequences
      std::vector<int> nbytes;

      /// pitch in bytes (distance between samples of the same sequence)
      int pitch;

      /// number of elements per sequence
      unsigned  int nElements;

  };

}// namespace mtca4u

#endif /* SOURCE_DIRECTORY__INCLUDE_DUMMYREGISTER_H_ */
