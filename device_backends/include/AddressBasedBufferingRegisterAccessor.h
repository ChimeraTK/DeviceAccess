/*
 * BufferingRegisterAccessor.h
 *
 *  Created on: Sep 28, 2015
 *      Author: Martin Hierholzer <martin.hierholzer@desy.de>
 */

#ifndef ADDRESSBASEDBUFFERINGREGISTERACCESSOR_H_
#define ADDRESSBASEDBUFFERINGREGISTERACCESSOR_H_

#include <vector>

#include "BufferingRegisterAccessor.h"
#include "DeviceBackend.h"
#include "FixedPointConverter.h"

namespace mtca4u {

  /*********************************************************************************************************************/
  /** Accessor class to read and write registers transparently by using the accessor object like a variable of the
   *  type UserType. Conversion to and from the UserType will be handled by the FixedPointConverter matching the
   *  register description in the map. Obtain the accessor using the Device::getBufferingRegisterAccessor() function.
   *
   *  Note: Transfers between the device and the internal buffer need to be triggered using the read() and write()
   *  functions before reading from resp. after writing to the buffer using the operators.
   */
  template<typename T>
  class AddressBasedBufferingRegisterAccessor : public BufferingRegisterAccessor<T> {
    public:

      /** Constructer. @attention Do not normally use directly.
       *  Users should call Device::getBufferingRegisterAccessor() to obtain an instance instead.
       */
      AddressBasedBufferingRegisterAccessor(boost::shared_ptr<DeviceBackend> dev, RegisterInfoMap::RegisterInfo &registerInfo)
        : BufferingRegisterAccessor<T>(dev),
          _registerInfo(registerInfo)
      {
        fixedPointConverter = FixedPointConverter(registerInfo.width, registerInfo.nFractionalBits, registerInfo.signedFlag);
        rawBuffer.resize(registerInfo.nElements);
        BufferingRegisterAccessor<T>::cookedBuffer.resize(registerInfo.nElements);
      }

      /** Placeholder constructer, to allow late initialisation of the accessor, e.g. in the open function.
       *  @attention Accessors created with this constructors will be dysfunctional!
       */
      AddressBasedBufferingRegisterAccessor() {}

      /** destructor
       */
      virtual ~AddressBasedBufferingRegisterAccessor() {}

      /** Return number of elements
       */
      inline unsigned int getNumberOfElements() {
        return rawBuffer.size();
      }

      /** Read the data from the device, convert it and store in buffer.
       */
      virtual void read() {
        BufferingRegisterAccessor<T>::_dev->read(_registerInfo.bar, _registerInfo.address, rawBuffer.data(),
            getNumberOfElements() * sizeof(uint32_t));
        for(unsigned int i = 0; i < getNumberOfElements(); i++) {
          BufferingRegisterAccessor<T>::cookedBuffer[i] = fixedPointConverter.template toCooked<T>(rawBuffer[i]);
        }
      }

      /** Convert data from the buffer and write to device.
       */
      virtual void write() {
        for(unsigned int i = 0; i < getNumberOfElements(); i++) {
          rawBuffer[i] = fixedPointConverter.toRaw(BufferingRegisterAccessor<T>::cookedBuffer[i]);
        }
        BufferingRegisterAccessor<T>::_dev->write(_registerInfo.bar, _registerInfo.address, rawBuffer.data(),
            getNumberOfElements() * sizeof(uint32_t));
      }

    protected:

      /// register information from the map
      RegisterInfoMap::RegisterInfo _registerInfo;

      /// fixed point converter
      FixedPointConverter fixedPointConverter;

      /// vector of unconverted data elements
      std::vector<int32_t> rawBuffer;

  };

}    // namespace mtca4u

#endif /* ADDRESSBASEDBUFFERINGREGISTERACCESSOR_H_ */
