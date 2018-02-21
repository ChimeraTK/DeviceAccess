/*
 * ForwardDeclarations.h
 *
 *  Created on: Mar 16, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_FORWARD_DECLARATIONS_H
#define MTCA4U_FORWARD_DECLARATIONS_H

#include <vector>

namespace ChimeraTK{

  class DeviceBackend;
  class Device;
  class TransferGroup;
  class RegisterAccessor;

  template< typename UserType >
  class BufferingRegisterAccessor;

  template< typename UserType >
  class NDRegisterAccessor;

  template<typename UserType>
  class TwoDRegisterAccessor;

  template<typename UserType>
  class MultiplexedDataAccessor;

  template<typename T>
  class DynamicValue;

}

#endif /* MTCA4U_FORWARD_DECLARATIONS_H */
