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

  /** Group several TransferElements to allow waiting for an update of any of the TransferElements. */
  class ReadAnyGroup {

    public:

      /** Construct empty group */
      ReadAnyGroup();

      /** Add TransferElement to group. Note that calling this function is only allowed before finalise() has been
       *  called. The given TransferElement may not yet be part of a ReadAnyGroup or a TransferGroup, otherwise an
       *  exception is thrown.
       *  The TransferElement must be must be readable. */
      void add(TransferElementAbstractor &element);

      /** Finalise the group. From this point on, add() may no longer be called. Only after the group has been finalised
       *  the read functions of this group may be called. Also, after the group has been finalised, read functions may
       *  no longer be called directly on the participating elements (including other copies of the same element).
       *
       *  The order of update notifications will only be well-defined for updates which happen after the call to
       *  finalise(). Any unread values which are present in the TransferElements when this function is called will not
       *  be processed in the correct sequence. Only the sequence within each TransferElement can be guaranteed. For any
       *  updates which arrive after the call to finalise() the correct sequence will be guaranteed even accross
       *  TransferElements.
       *
       *  This function will call readAsync() on all elements in the group. For elements which have not been created
       *  with TransferModeFlag::wait_for_new_data, readAsync() must be called to initiate new transfers when needed. */
      void finalise();

      /** Wait until one of the elements in this group has received an update. The function will return the
       *  TransferElementID of the element which has received the update. If multiple updates are received at the same
       *  time or if multiple updates were already present before the call to this function, the ID of the first element
       *  receiving an update will be returned.
       *
       *  Before returning, the postRead action will be called on the TransferElement whose ID is returned, so the read
       *  data will already be present in the user buffer. All other TransferElements in this group will not be
       *  altered.
       *
       *  Before calling this function, finalise() must have been called, otherwise the behaviour is undefined. */
      TransferElementID waitAny();

      /** Wait until the given TransferElement has received an update and store it to its user buffer. All updates of
       *  other elements which are received before the update of the given element will be processed and are thus
       *  visible in the user buffers when this function returns.
       *
       *  The specified TransferElement must be part of this ReadAnyGroup, otherwise the behaviour is undefined.
       *
       *  This is merely a convenience function calling waitAny() in a loop until the ID of the given element is
       *  returned.
       *
       *  Before calling this function, finalise() must have been called, otherwise the behaviour is undefined. */
      void waitUntil(TransferElementID id);
      void waitUntil(TransferElementAbstractor &element);

    private:

      bool isFinalised{false};

      std::vector<TransferElementAbstractor> elements;

      cppext::future_queue<size_t> notification_queue;

  };

  /********************************************************************************************************************/

  inline ReadAnyGroup::ReadAnyGroup() {}

  /********************************************************************************************************************/

  inline void ReadAnyGroup::add(TransferElementAbstractor &element) {
    if(isFinalised) {
      throw std::logic_error("ReadAnyGroup has already been finalised, calling add() is no longer allowed.");
    }
    if(!element.isReadable()) {
      throw std::logic_error("Cannot add non-readable accessor for register "+element.getName()+" to ReadAnyGroup.");
    }
    elements.push_back(element);
  }

  /********************************************************************************************************************/

  inline void ReadAnyGroup::finalise() {
    if(isFinalised) throw std::logic_error("ReadAnyGroup has already been finalised, calling finalise() is no longer allowed.");
    std::vector<cppext::future_queue<void>> queueList;
    for(auto &e : elements) {
      auto tf = e.readAsync();
      queueList.push_back( detail::getFutureQueueFromTransferFuture(tf) );
    }
    notification_queue = cppext::when_any(queueList.begin(), queueList.end());
    isFinalised = true;
  }

  /********************************************************************************************************************/

  inline TransferElementID ReadAnyGroup::waitAny() {
    size_t idx;
    notification_queue.pop_wait(idx);
    elements[idx].readAsync().wait();
    return elements[idx].getId();
  }

  /********************************************************************************************************************/

  inline void ReadAnyGroup::waitUntil(TransferElementID id) {
    while(true) {
      auto read = waitAny();
      if(read == id) return;
    }
  }

  /********************************************************************************************************************/

  inline void ReadAnyGroup::waitUntil(TransferElementAbstractor &element) {
    waitUntil(element.getId());
  }

  /********************************************************************************************************************/

} /* namespace ChimeraTK */

#endif // CHIMERATK_READ_ANY_H
