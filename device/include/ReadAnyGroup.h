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

  /** Group several registers (= TransferElement) to allow waiting for an update of any of the registers. */
  class ReadAnyGroup {

    public:

      /** Construct empty group. Elements can later be added using the add() function, or by copying another object. */
      ReadAnyGroup();

      /** Construct finalised group with the given elements. The group will behave like finalise() had already been
       *  called. */
      ReadAnyGroup(std::initializer_list<TransferElementAbstractor> list);

      template<typename ITERATOR>
      ReadAnyGroup(ITERATOR first, ITERATOR last);

      /** Add register to group. Note that calling this function is only allowed before finalise() has been
       *  called. The given register may not yet be part of a ReadAnyGroup or a TransferGroup, otherwise an
       *  exception is thrown.
       *
       *  The register must be must be readable. */
      void add(TransferElementAbstractor element);

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
       *  This function will call readAsync() on all elements with AccessMode::wait_for_new_data in the group. There
       *  must be at least one transfer element with AccessMode::wait_for_new_data in the group, otherwise an exception
       *  is thrown. */
      void finalise();

      /** Wait until one of the elements in this group has received an update. The function will return the
       *  TransferElementID of the element which has received the update. If multiple updates are received at the same
       *  time or if multiple updates were already present before the call to this function, the ID of the first element
       *  receiving an update will be returned.
       *
       *  Only elements with AccessMode::wait_for_new_data are used for waiting. Once an update has been received for
       *  one of these elements, the function will call readLatest() on all elements without
       *  AccessMode::wait_for_new_data.
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

      /// Flag if this group has been finalised already
      bool isFinalised{false};

      /// Vector of elements in this group
      std::vector<TransferElementAbstractor> elements;

      /// The notification queue, will be valid only if isFinalised == true
      cppext::future_queue<size_t> notification_queue;

      /// Element to call the transferFutureWaitCallback on - this is the first element with wait_for_new_data
      TransferElementAbstractor elementToCallTransferFutureWaitCallback;

  };

  /********************************************************************************************************************/

  inline ReadAnyGroup::ReadAnyGroup() {}

  /********************************************************************************************************************/

  inline ReadAnyGroup::ReadAnyGroup(std::initializer_list<TransferElementAbstractor> list) {
    for(auto &element : list) add(element);
    finalise();
  }

  /********************************************************************************************************************/

  template<typename ITERATOR>
  ReadAnyGroup::ReadAnyGroup(ITERATOR first, ITERATOR last) {
    for(ITERATOR it = first; it != last; ++it) add(*it);
    finalise();
  }

  /********************************************************************************************************************/

  inline void ReadAnyGroup::add(TransferElementAbstractor element) {
    if(isFinalised) {
      throw std::logic_error("ReadAnyGroup has already been finalised, calling add() is no longer allowed.");
    }
    if(!element.isReadable()) {
      throw std::logic_error("Cannot add non-readable accessor for register "+element.getName()+" to ReadAnyGroup.");
    }
    elements.push_back(element);
    if( !elementToCallTransferFutureWaitCallback.isInitialised() &&
        element.getAccessModeFlags().has(AccessMode::wait_for_new_data) ) {
      elementToCallTransferFutureWaitCallback = element;
    }
  }

  /********************************************************************************************************************/

  inline void ReadAnyGroup::finalise() {
    if(isFinalised) throw std::logic_error("ReadAnyGroup has already been finalised, calling finalise() is no longer allowed.");
    std::vector<cppext::future_queue<void>> queueList;
    bool groupEmpty = true;
    for(auto &e : elements) {
      if(e.getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
        auto tf = e.readAsync();
        queueList.push_back( detail::getFutureQueueFromTransferFuture(tf) );
        groupEmpty = false;
      }
    }
    if(groupEmpty) {
      throw std::logic_error("ReadAnyGroup has no element with AccessMode::wait_for_new_data.");
    }
    notification_queue = cppext::when_any(queueList.begin(), queueList.end());
    isFinalised = true;
  }

  /********************************************************************************************************************/

  inline TransferElementID ReadAnyGroup::waitAny() {
    size_t idx;
    elementToCallTransferFutureWaitCallback.transferFutureWaitCallback();
    notification_queue.pop_wait(idx);
    elements[idx].readAsync().wait();
    for(auto &e : elements) {
      if(!e.getAccessModeFlags().has(AccessMode::wait_for_new_data)) e.readLatest();
    }
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
