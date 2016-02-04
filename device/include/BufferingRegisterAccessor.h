/*
 * BufferingRegisterAccessor.h
 *
 *  Created on: Sep 28, 2015
 *      Author: Martin Hierholzer <martin.hierholzer@desy.de>
 */

#ifndef MTCA4U_BUFFERING_REGISTER_ACCESSOR_H
#define MTCA4U_BUFFERING_REGISTER_ACCESSOR_H

#include <vector>

#include "FixedPointConverter.h"

namespace mtca4u {

  class DeviceBackend;

  /*********************************************************************************************************************/
  /** Accessor class to read and write registers transparently by using the accessor object like a variable of the
   *  type UserType. Conversion to and from the UserType will be handled by the FixedPointConverter matching the
   *  register description in the map. Obtain the accessor using the Device::getBufferingRegisterAccessor() function.
   *
   *  Note: Transfers between the device and the internal buffer need to be triggered using the read() and write()
   *  functions before reading from resp. after writing to the buffer using the operators.
   */
  template<typename T>
  class BufferingRegisterAccessor {
    public:

      /** Constructer. @attention Do not normally use directly.
       *  Users should call Device::getBufferingRegisterAccessor() to obtain an instance instead.
       */
      BufferingRegisterAccessor(boost::shared_ptr<DeviceBackend> dev, const std::string &module, const std::string &registerName)
      {
        _accessor = dev->getRegisterAccessor(registerName, module);
        cookedBuffer.resize(_accessor->getNumberOfElements());
      }

      /** Placeholder constructer, to allow late initialisation of the accessor, e.g. in the open function.
       *  @attention Accessors created with this constructors will be dysfunctional!
       */
      BufferingRegisterAccessor() {}

      /** destructor
       */
      virtual ~BufferingRegisterAccessor() {};


      /** Read the data from the device, convert it and store in buffer.
       */
      virtual void read() {
        _accessor->read(cookedBuffer.data(), getNumberOfElements());
      }

      /** Convert data from the buffer and write to device.
       */
      virtual void write() {
        _accessor->write(cookedBuffer.data(), getNumberOfElements());
      }

      /** Get or set buffer content by [] operator.
       *  @attention No bounds checking is performed, use getNumberOfElements() to obtain the number of elements in
       *  the register.
       */
      inline T& operator[](unsigned int index) {
        return cookedBuffer[index];
      }

      /** Return number of elements
       */
      inline unsigned int getNumberOfElements() {
        return cookedBuffer.size();
      }

      /** Access data with std::vector-like iterators
       */
      typedef typename std::vector<T>::iterator iterator;
      typedef typename std::vector<T>::const_iterator const_iterator;
      typedef typename std::vector<T>::reverse_iterator reverse_iterator;
      typedef typename std::vector<T>::const_reverse_iterator const_reverse_iterator;
      inline iterator begin() { return cookedBuffer.begin(); }
      inline const_iterator begin() const { return cookedBuffer.begin(); }
      inline iterator end() { return cookedBuffer.end(); }
      inline const_iterator end() const { return cookedBuffer.end(); }
      inline reverse_iterator rbegin() { return cookedBuffer.rbegin(); }
      inline const_reverse_iterator rbegin() const { return cookedBuffer.rbegin(); }
      inline reverse_iterator rend() { return cookedBuffer.rend(); }
      inline const_reverse_iterator rend() const { return cookedBuffer.rend(); }

      /* Swap content of (cooked) buffer with std::vector
       */
      inline void swap(std::vector<T> &x) {
        cookedBuffer.swap(x);
      }

      /** Factory function as called by Device::getBufferingRegisterAccessor() and Device::getCustomAccessor()
       */
      static BufferingRegisterAccessor<T> createInstance(
          std::string const &dataRegionName, std::string const &module, boost::shared_ptr<DeviceBackend> const &backend,
          boost::shared_ptr<RegisterInfoMap> const &) {
        return backend->template getBufferingRegisterAccessor<T>(module, dataRegionName);
      }

    protected:

      /// pointer to non-buffering accessor
      boost::shared_ptr< RegisterAccessor > _accessor;

      /// vector of converted data elements
      std::vector<T> cookedBuffer;

  };

}    // namespace mtca4u

#endif /* MTCA4U_BUFFERING_REGISTER_ACCESSOR_H */
