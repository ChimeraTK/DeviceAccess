/*
 * TransferElement.h
 *
 *  Created on: Feb 11, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_TRANSFER_ELEMENT_H
#define MTCA4U_TRANSFER_ELEMENT_H

#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace mtca4u {

  // forward declaration to make a friend
  class TransferGroup;

  /** Base class for register accessors which can be part of a TransferGroup */
  class TransferElement : public boost::enable_shared_from_this<TransferElement> {

    public:

      /** Abstract base classes need a virtual destructor */
      virtual ~TransferElement() {}

      /** Read the data from the device. */
      virtual void read() = 0;

      /** Write the data to device. */
      virtual void write() = 0;

      /** Check if the two TransferElements are identical, i.e. accessing the same hardware register */
      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const = 0;

      /** Check if transfer element is read only */
      virtual bool isReadOnly() const = 0;

      /** Obtain the underlying TransferElements with actual hardware access. If this transfer element
       *  is directly reading from / writing to the hardware, it will return a list just containing
       *  a shared pointer of itself.
       */
      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() = 0;

      /** Search for all underlying TransferElements which are considered identicel (see sameRegister()) with
       *  the given TransferElement. These TransferElements are then replaced with the new element. If no underlying
       *  element matches the new element, this function has no effect.
       */
      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) = 0;

      friend class TransferGroup;

  };

} /* namespace mtca4u */

#endif /* MTCA4U_TRANSFER_ELEMENT_H */
