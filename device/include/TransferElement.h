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

namespace mtca4u {

  // forward declaration to make a friend
  class TransferGroup;

  /** Base class for register accessors which can be part of a TransferGroup */
  class TransferElement {

    public:

      /** Abstract base classes need a virtual destructor */
      virtual ~TransferElement() {}

      /** Read the data from the device. */
      virtual void read() = 0;

      /** Write the data to device. */
      virtual void write() = 0;

      /** Check if the two TransferElements are identical, i.e. accessing the same hardware register */
      virtual bool operator==(const TransferElement &rightHandSide) const = 0;

    protected:

      /** Obtain the underlying TransferElements with actual hardware access. If this transfer element
       *  is directly reading from / writing to the hardware, it will return an empty list. In case of e.g. a
       *  StructRegisterAccessor, a list of the actual transfer elements will be returned.
       *
       *  @attention: The list is returned as pointers to shared pointers, since the TransferGroup must be able
       *  to replace the used TransferElements with others. Do not use this function outside the TransferGroup class!
       *
       *  @todo is there a better way to do this?
       */
      virtual std::vector< boost::shared_ptr<TransferElement>* > getHardwareAccessingElements() {
        return std::vector< boost::shared_ptr<TransferElement>* >();
      }

      friend class TransferGroup;

  };

} /* namespace mtca4u */

#endif /* MTCA4U_TRANSFER_ELEMENT_H */
