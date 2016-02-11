/*
 * TransferGroup.h
 *
 *  Created on: Feb 11, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_TRANSFER_GROUP_H
#define MTCA4U_TRANSFER_GROUP_H

#include <vector>

#include "TransferElement.h"

namespace mtca4u {

  // forward declarations
  template<typename UserType>
  class BufferingRegisterAccessor;

  template<typename UserType>
  class TwoDRegisterAccessor;

  /** Group multiple data accessors to efficiently trigger data transfers on the whole group. In case of some backends
   *  like the LogicalNameMappingBackend, grouping data accessors can avoid unnecessary transfers of the same data.
   *  This happens in particular, if accessing the data of one accessor requires transfer of a bigger block of
   *  data containing also data of another accessor (e.g. channel accessors for multiplexed 2D registers).
   *
   *  Grouping accessors can only work with accessors that internally buffer the transferred data. Therefore the
   *  standard RegisterAccessor is not supported, as its read() and write() functions always directly read from/write to
   *  the hardware. Only the BufferingRegisterAccessor, the TwoDRegisterAccessor and the StructRegisterAccessor
   *  are supported. Note that read() and write() of the accessors put into the group should no longer be used, since
   *  they would perform the hardware access directly. Instead, read() and write() of the TransferGroup should be
   *  called.
   */
  class TransferGroup {

    public:

      TransferGroup() {};
      ~TransferGroup() {};

      /** Add a register accessor to the group. This function is overloaded for all supported types of accessors. */
      template<typename UserType>
      void addAccessor(BufferingRegisterAccessor<UserType> &accessor) {
        auto newElements = accessor._impl->getHardwareAccessingElements();
        if(newElements.empty()) newElements.push_back( (boost::shared_ptr<TransferElement>*) &accessor._impl);
        uniqueInsert(newElements);
      }

      template<typename UserType>
      void addAccessor(TwoDRegisterAccessor<UserType> &accessor) {
        auto newElements = accessor._impl->getHardwareAccessingElements();
        if(newElements.empty()) newElements.push_back( (boost::shared_ptr<TransferElement>*) &accessor._impl);
        uniqueInsert(newElements);
      }

      /** Trigger read transfer for all accessors in the group */
      void read();

      /** Trigger write transfer for all accessors in the group */
      void write();

    protected:

      /** List of TransferElements in this group */
      std::vector< boost::shared_ptr<TransferElement> > elements;

      /** Insert elements to the list if no equal element is already in the list. Already present elements
       *  will be replaced at their origin (i.e. inside the register accessor!) */
      void uniqueInsert(std::vector< boost::shared_ptr<TransferElement>* > &newElements);

  };

} /* namespace mtca4u */

#endif /* MTCA4U_TRANSFER_GROUP_H */
