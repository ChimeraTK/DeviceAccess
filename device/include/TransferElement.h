/*
 * TransferElement.h
 *
 *  Created on: Feb 11, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_TRANSFER_ELEMENT_H
#define MTCA4U_TRANSFER_ELEMENT_H

#include <vector>
#include <string>
#include <typeinfo>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "DeviceException.h"
#include "TimeStamp.h"

namespace mtca4u {

  // forward declaration to make a friend
  class TransferGroup;

  /** Base class for register accessors which can be part of a TransferGroup */
  class TransferElement : public boost::enable_shared_from_this<TransferElement>{

    public:
      /** Creates a transfer element with the specified name. */
      TransferElement(std::string const & name = std::string()) 
        : _name(name), isInTransferGroup(false) {}
      
      /** Abstract base classes need a virtual destructor. */
      virtual ~TransferElement() {}

      /** Returns the name that identifies the process variable. */
      const std::string& getName() const{
	return _name;
      }

      /** Returns the \c std::type_info for the value type of this transfer element.
       *  This can be used to determine the type at runtime.
       */
      virtual const std::type_info& getValueType() const = 0;

      /** Read the data from the device. */
      virtual void read() = 0;

      /** Read the next value, if available in the input buffer.
       *  This function returns immediately and the return value indicated
       *  if a new value was available (<true>) or not (<code>false</code>).
       *
       *  This function throws a DeviceException with ID NOT_AVAILABLE if 
       *  the backend does not support non-blocking reads.
       */
      virtual bool readNonBlocking() = 0;

      /** Write the data to device. */
      virtual void write() = 0;

      /** Called by the TransferGroup after a read was executed directly on the underlying accessor. This function
       *  must be implemented to extract the read data from the underlying accessor and expose it to the user. */
      virtual void postRead() {};

      /** Called by the TransferGroup before a write will be executed directly on the underlying accessor. This
       *  function implemented be used to transfer the data to be written into the underlying accessor. */
      virtual void preWrite() {};

      /** Called by the TransferGroup after a write will be executed directly on the underlying accessor. */
      virtual void postWrite() {};

      /** Check if the two TransferElements are identical, i.e. accessing the same hardware register */
      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const = 0;

      /** Check if transfer element is read only, i\.e\. it is readable but not writeable. */
      virtual bool isReadOnly() const = 0;

      /** Check if transfer element is readable. It throws an acception if you try to read and 
       *  isReadable() is not true.*/
      virtual bool isReadable() const = 0;
      
      /** Check if transfer element is writeable. It throws an acception if you try to write and 
       *  isWriteable() is not true.*/
      virtual bool isWriteable() const = 0;
      
      /** @brief Deprecated, do not use
       *  @deprecated The time stamp will be replaced with a unique counter.
       *  Only used for backward compatibility with the control system adapter. All implementations
       *  in DeviceAccess will throw an exception with DeviceException::NOT_IMPLEMENTED.
       *
       *  Returns the time stamp associated with the current value of the transfer element.
       *  Typically, this is the time when the value was updated.
       */
      virtual TimeStamp getTimeStamp() const{
        throw DeviceException("getTimeStamp is not implemented in DeviceAccess.", DeviceException::NOT_IMPLEMENTED);
      }

      /** @brief Deprecated, do not use
       *  @deprecated Only used for backward compatibility with the control system adapter.
       *  This feature will be removed soon, maybe even before the next tag. DO NOT USE IT!!
       */
      virtual bool isArray() {
        throw DeviceException("isArray is deprecated and intentionally not implemented in DeviceAccess.", DeviceException::NOT_IMPLEMENTED);
      }

      /** Obtain the underlying TransferElements with actual hardware access. If this transfer element
       *  is directly reading from / writing to the hardware, it will return a list just containing
       *  a shared pointer of itself.
       */
      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() = 0;

      /** Obtain the highest level implementation TransferElement. For TransferElements which are itself an
       *  implementation this will directly return a shared pointer to this. If this TransferElement is a user
       *  frontend, the pointer to the internal implementation is returned. */
      virtual boost::shared_ptr<TransferElement> getHighLevelImplElement() {
        return shared_from_this();
      }

      /** Search for all underlying TransferElements which are considered identicel (see sameRegister()) with
       *  the given TransferElement. These TransferElements are then replaced with the new element. If no underlying
       *  element matches the new element, this function has no effect.
       */
      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) = 0;

      friend class TransferGroup;

    protected:
      /** Identifier uniquely identifying the TransferElement */
      std::string _name;      

      bool isInTransferGroup;
  };

} /* namespace mtca4u */

#endif /* MTCA4U_TRANSFER_ELEMENT_H */
