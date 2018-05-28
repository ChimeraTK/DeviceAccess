/*
 * ReadAny.h
 *
 *  Created on: Dec 19, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_READ_ANY_H
#define CHIMERATK_READ_ANY_H

#include <ChimeraTK/cppext/future_queue.hpp>

#include "TransferElementAbstractor.h"

namespace ChimeraTK {

  /** Group several TransferElements to allow waiting for an update of any of the TransferElements. The TransferElements
   *  must be created with the AccessModeFlag::wait_for_new_data and must be readable. */
  class ReadAnyGroup {

    public:

      /** Construct empty group */
      ReadAnyGroup();

      /** Add TransferElement to group. Note that calling this function is only allowed before finalise() has been
       *  called. The given TransferElement may not yet be part of a ReadAnyGroup or a TransferGroup, otherwise an
       *  exception is thrown. */
      void add(TransferElementAbstractor &element);

      /** Finalise the group. From this point on, add() may no longer be called. Only after the group has been finalised
       *  the read functions of this group may be called. Also, after the group has been finalised, read functions may
       *  no longer be called directly on the participating elements (including other copies of the same element).
       *
       *  The order of update notifications will only be well-defined for updates which happen after the call to
       *  finalise(). Any unread values which are present in the TransferElements when this function is called will not
       *  be processed in the correct sequence. Only the sequence within each TransferElement can be guaranteed. For any
       *  updates which arrive after the call to finalise() the correct sequence will be guaranteed even accross
       *  TransferElements. */
      void finalise();

      /** Wait until one of the elements in this group has received an update. The function will return the
       *  TransferElementID of the element which has received the update. If multiple updates are received at the same
       *  time or if multiple updates were already present before the call to this function, the ID of the first element
       *  receiving an update will be returned.
       *
       *  Before returning, the postRead action will be called on the TransferElement whose ID is returned, so the read
       *  data will already be present in the user buffer. All other TransferElements in this group will not be
       *  altered. */
      TransferElementID waitAny();

      /** Wait until the given TransferElement has received an update and store it to its user buffer. All updates of
       *  other elements which are received before the update of the given element will be processed and are thus
       *  visible in the user buffers when this function returns.
       *
       *  The specified TransferElement must be part of this ReadAnyGroup, otherwise an exception is thrown.
       *
       *  This is merely a convenience function calling waitAny() in a loop until the ID of the given element is
       *  returned. */
      void waitUntil(TransferElementAbstractor &element);
      void waitUntil(TransferElementID id);

    private:

      bool isFinalised{false};



      cppext::future_queue<void> notification_queue;

  };

  /********************************************************************************************************************/

  inline ReadAnyGroup::ReadAnyGroup() {}


} /* namespace ChimeraTK */

#endif // CHIMERATK_READ_ANY_H
