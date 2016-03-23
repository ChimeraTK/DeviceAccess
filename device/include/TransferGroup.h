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

  /** Group multiple data accessors to efficiently trigger data transfers on the whole group. In case of some backends
   *  like the LogicalNameMappingBackend, grouping data accessors can avoid unnecessary transfers of the same data.
   *  This happens in particular, if accessing the data of one accessor requires transfer of a bigger block of
   *  data containing also data of another accessor (e.g. channel accessors for multiplexed 2D registers).
   *
   *  Grouping accessors can only work with accessors that internally buffer the transferred data. Therefore the
   *  standard RegisterAccessor is not supported, as its read() and write() functions always directly read from/write to
   *  the hardware. Note that read() and write() of the accessors put into the group should no longer be used, since
   *  they would perform the hardware access directly. Instead, read() and write() of the TransferGroup should be
   *  called.
   */
  class TransferGroup {

    public:

      TransferGroup()
      : readOnly(false)
      {};
      ~TransferGroup() {};

      /** Add a register accessor to the group. The register accessor might internally be altered so that accessors
       *  accessing the same hardware register will share their buffers. Register accessors must not be placed into
       *  multiple TransferGroups.
       */
      template<class AnyBufferingAccessorType>
      void addAccessor(AnyBufferingAccessorType &accessor) {
        // just call the template specialisation for TransferElement, which we actually implement
        addAccessor<TransferElement>(*(accessor.getHighLevelImplElement()));
      }

      /** Trigger read transfer for all accessors in the group */
      void read();

      /** Trigger write transfer for all accessors in the group */
      void write();

      /** Check if transfer group is read-only. A transfer group is read-only, if at least one of its transfer
       *  elements is read-only. */
      bool isReadOnly();

    protected:

      /** List of low-level TransferElements in this group, which are directly responsible for the hardware access */
      std::vector< boost::shared_ptr<TransferElement> > elements;

      /** List of high-level TransferElements in this group which are directly used by the user */
      std::vector< boost::shared_ptr<TransferElement> > highLevelElements;

      /** Flag if group is read-only */
      bool readOnly;
  };

  /** Template specialisation for adding a TransferElement. TransferElement is a base class of the supported register
   *  accessor classes but normally inaccessible due to protected inheritance. Thus the user could not call this
   *  function without this template trick. The TransferGroup class need to be friend of the accessor classes!
    */
  template<>
  void TransferGroup::addAccessor<TransferElement>(TransferElement &accessor);

} /* namespace mtca4u */

#endif /* MTCA4U_TRANSFER_GROUP_H */
