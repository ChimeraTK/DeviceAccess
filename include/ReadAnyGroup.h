// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "TransferElement.h"
#include "TransferElementAbstractor.h"

#include <ChimeraTK/cppext/future_queue.hpp>

#include <functional>
#include <initializer_list>

namespace ChimeraTK {

  namespace DataConsistencyGroupDetail {
    class HistorizedMatcher;
  } // namespace DataConsistencyGroupDetail

  /**
   * Group several registers (= TransferElement) to allow waiting for an update of any of the registers.
   *
   * After the group has been finalised (cf. ReadAnyGroup::finalise), read functions may no longer be called directly on
   * the participating elements.
   */
  class ReadAnyGroup {
    // friend because it needs to decorate our push_elements
    friend class ChimeraTK::DataConsistencyGroupDetail::HistorizedMatcher;

   public:
    /**
     * Notification object returned by waitAny(). A notification can be accepted immediately or retained to be accepted
     * at a later point in time.
     */
    class Notification {
     public:
      friend class ReadAnyGroup;

      /**
       * Create an empty notification. Such a notification only acts as a place-holder that can be assigned from another
       * notification and is invalid until it is assigned.
       */
      Notification();

      /**
       * Construct a notification from another notification. After this, this notification will contain the state and
       * information of the other notification and the other notification is going to be invalid.
       */
      Notification(Notification&& other) noexcept;

      /**
       * Destructor. Calls accept() if this notification is valid and has not been accepted yet.
       */
      ~Notification();

      /**
       * Assign this notification from another notification.  After this, this notification will contain the state and
       * information of the other notification and the other notification is going to be invalid. If this notification
       * was valid and had not been accepted before, accept() is called before assigning the data from the other
       * notification.
       */
      Notification& operator=(Notification&& other) noexcept;

      /**
       * Accept the notification. This will complete the read operation of the transfer element for which this
       * notification has been generated. After accepting a notification, this notification object becomes invalid.
       *
       * Due to implementation details, it can happen that a notification is generated without a new value being
       * actually available. In these cases, this method returns false and the transfer element is not updated with a
       * new value. In all other cases, this method returns true.
       *
       * This method throws an ChimeraTK::logic_error if this method is called on an invalid notification or a
       * notification that has already been accepted.
       */
      bool accept();

      /**
       * Return the ID of the transfer element for which this notification has been generated.
       *
       * This method throws an ChimeraTK::logic_error if this method is called on an invalid notification.
       */
      [[nodiscard]] TransferElementID getId();

      /**
       * Return the index of the transfer element for which this notifiaction has been generated. The index is the
       * offset into the list of transfer elements that was specified when creating the ReadAnyGroup.
       *
       * This method throws an ChimeraTK::logic_error if this method is called on an invalid notification.
       */
      [[nodiscard]] std::size_t getIndex() const;

      /**
       * Return the transfer element for which this notification has been generated.
       *
       * This method throws an ChimeraTK::logic_error if this method is called on an invalid notification.
       */
      [[nodiscard]] TransferElementAbstractor getTransferElement();

      /**
       * Tell whether this notification is valid and has not been accepted yet.
       */
      [[nodiscard]] bool isReady() const;

      /*
       * Notifications cannot be copied because each notification can only be accepted once.
       */
      Notification(const Notification&) = delete;
      Notification& operator=(const Notification&) = delete;

     private:
      /**
       * Private constructor used by ReadAnyGroup::waitAny(). This is the only constructor that can construct a new,
       * valid notification.
       */
      Notification(std::size_t index, ReadAnyGroup* owner);

      /// Flag indicating whether accept() has been called.
      bool accepted{false};

      /// Index of the transfer element in the list of transfer elements.
      std::size_t index{};

      /// Flag indicating whether this notification is valid.
      bool valid{false};

      /// Pointer to owning ReadAnyGroup
      ReadAnyGroup* _owner{nullptr};
    };

    /**
     * Construct empty group. Elements can later be added using the add() function, or by copying another object.
     */
    ReadAnyGroup();

    /**
     * Construct finalised group with the given elements. The group will behave like finalise() had already been called.
     */
    ReadAnyGroup(std::initializer_list<std::reference_wrapper<TransferElementAbstractor>> list);

    /**
     * Construct finalised group with the given elements. The group will behave like finalise() had already been called.
     */
    ReadAnyGroup(std::initializer_list<boost::shared_ptr<TransferElement>> list);

    /**
     * Construct finalised group with the given elements. The group will behave like finalise() had already been called.
     */
    template<typename ITERATOR>
    ReadAnyGroup(ITERATOR first, ITERATOR last);

    /**
     * Add register to group. Note that calling this function is only allowed before finalise() has been called. The
     * given register may not yet be part of a ReadAnyGroup or a TransferGroup, otherwise an exception is thrown.
     *
     * The register must be must be readable.
     * Note, we disallow adding const-refs to TransferElement(Abtractor)s.
     * TransferElements added to the group change in behaviour, since their underlying future_queues are modified (in
     * order to notify the ReadAny future queue).
     */
    void add(TransferElementAbstractor& element);

    ReadAnyGroup(const ReadAnyGroup&) = delete;
    ReadAnyGroup(ReadAnyGroup&& other) noexcept { operator=(std::move(other)); }
    ReadAnyGroup& operator=(ReadAnyGroup&& other) noexcept;

    /**
     * See the other signature of add().
     */
    void add(boost::shared_ptr<TransferElement> element);

    /**
     * Finalise the group. From this point on, add() may no longer be called. Only after the group has been finalised
     * the read functions of this group may be called. Also, after the group has been finalised, read functions may no
     * longer be called directly on the participating elements (including other copies of the same element).
     *
     * The order of update notifications will only be well-defined for updates which happen after the call to
     * finalise(). Any unread values which are present in the TransferElements when this function is called will not be
     * processed in the correct sequence. Only the sequence within each TransferElement can be guaranteed. For any
     * updates which arrive after the call to finalise() the correct sequence will be guaranteed even accross
     * TransferElements.
     *
     * This function will call readAsync() on all elements with AccessMode::wait_for_new_data in the group. There must
     * be at least one transfer element with AccessMode::wait_for_new_data in the group, otherwise an exception is
     * thrown.
     */
    void finalise();

    /**
     * Wait until one of the elements in this group has received an update. The function will return the
     * TransferElementID of the element which has received the update. If multiple updates are received at the same time
     * or if multiple updates were already present before the call to this function, the ID of the first element
     * receiving an update will be returned.
     *
     * Only elements with AccessMode::wait_for_new_data are used for waiting. Once an update has been received for one
     * of these elements, the function will call readLatest() on all elements without AccessMode::wait_for_new_data
     * (this is equivalent to calling processPolled()).
     *
     * Before returning, the postRead action will be called on the TransferElement whose ID is returned, so the read
     * data will already be present in the user buffer. All other TransferElements in this group will not be altered.
     *
     * Before calling this function, finalise() must have been called, otherwise the behaviour is undefined.
     */
    TransferElementID readAny();

    /**
     * Read the next available update in the group, but do not block if no update is available. If no update is
     * available, a default-constructed TransferElementID is returned after all poll-type elements in the group have
     * been updated.
     *
     * Before calling this function, finalise() must have been called, otherwise the behaviour is undefined.
     */
    TransferElementID readAnyNonBlocking();

    /**
     * Wait until the given TransferElement has received an update and store it to its user buffer. All updates of other
     * elements which are received before the update of the given element will be processed and are thus visible in the
     * user buffers when this function returns.
     *
     * The specified TransferElement must be part of this ReadAnyGroup, otherwise the behaviour is undefined.
     *
     * This is merely a convenience function calling waitAny() in a loop until the ID of the given element is returned.
     *
     * Before calling this function, finalise() must have been called, otherwise the behaviour is undefined.
     */
    void readUntil(const TransferElementID& id);

    /**
     * See the other signature of readUntil().
     */
    void readUntil(const TransferElementAbstractor& element);

    /**
     * Wait until all of the given TransferElements has received an update and store it to its user buffer. All updates
     * of other elements which are received before the update of the given element will be processed and are thus
     * visible in the user buffers when this function returns.
     *
     * The specified TransferElement must be part of this ReadAnyGroup, otherwise the behaviour is undefined.
     *
     * This is merely a convenience function calling waitAny() in a loop until the ID of the given element is returned.
     *
     * Before calling this function, finalise() must have been called, otherwise the behaviour is undefined.
     */
    void readUntilAll(const std::vector<TransferElementID>& ids);

    /**
     * See the other signature of readUntilAll().
     */
    void readUntilAll(const std::vector<TransferElementAbstractor>& elements);

    /**
     * Wait until one of the elements received an update notification, but do not actually process the updated value
     * yet. This is similar to readAny() but the caller has to call accept() on the returned object manually. Also the
     * poll-type elements in the group are not updated in this function.
     *
     * This allows e.g. to acquire a lock before executing accept().
     *
     * Before calling this function, finalise() must have been called, otherwise the behaviour is undefined.
     *
     * The returned Notification object is only valid as long as the ReadAnyGroup still exists.
     */
    Notification waitAny();

    /**
     * Check if an update is available in the group, but do not block if no update is available. If no update is
     * available, an invalid Notification object is returned (i.e. Notification::isReady() will return false).
     *
     * Before calling this function, finalise() must have been called, otherwise the behaviour is undefined.
     *
     * The returned Notification object is only valid as long as the ReadAnyGroup still exists.
     */
    Notification waitAnyNonBlocking();

    /**
     * Process polled transfer elements (update them if new values are available).
     *
     * Before calling this function, finalise() must have been called, otherwise the behaviour is undefined.
     */
    void processPolled();

    /**
     * Convenience function to interrupt any running readAny/waitAny by calling interrupt on one of the push-type
     * TransferElements in the group.
     */
    void interrupt() { push_elements.front().getHighLevelImplElement()->interrupt(); }

   private:
    /// Call preRead() on the push_elements which need it
    void handlePreRead();

    /// Flag if this group has been finalised already
    bool isFinalised{false};

    /// Vector of push-type elements in this group
    std::vector<TransferElementAbstractor> push_elements;

    /// Vector of poll-type elements in this group
    std::vector<TransferElementAbstractor> poll_elements;

    /// The notification queue, will be valid only if isFinalised == true
    cppext::future_queue<size_t> notification_queue;

    /// Index into push_elements pointing to the last operation's TransferElementAbstractor, or
    /// std::numeric_limits<size_t>::max() in case there was not yet an operation.
    /// This is used to call preRead() at the beginning of the next operation.
    size_t _lastOperationIndex{std::numeric_limits<size_t>::max()};
  };

  /********************************************************************************************************************/
  /* Implementations of ReadAnyGroup::Notification */
  /********************************************************************************************************************/

  inline ReadAnyGroup::Notification::Notification() = default;

  /********************************************************************************************************************/

  inline ReadAnyGroup::Notification::Notification(Notification&& other) noexcept
  : accepted(other.accepted), index(other.index), valid(other.valid), _owner(other._owner) {
    other.valid = false;
  }

  /********************************************************************************************************************/

  inline ReadAnyGroup::Notification::~Notification() {
    // It is important that each received notification is consumed. This means that we have to accept a notification
    // before we can destroy it.
    if(this->valid && !this->accepted) {
      try {
        this->accept();
      }
      catch(...) {
        // Do not let exceptions escape the destructor, rather terminate the application instead. This will still print
        // the exception message.
        std::terminate();
      }
    }
  }

  /********************************************************************************************************************/

  inline ReadAnyGroup::Notification& ReadAnyGroup::Notification::operator=(Notification&& other) noexcept {
    // It is important that each received notification is consumed. This means that we have to accept this notification
    // before we can overwrite it with another one.
    if(this->valid && !this->accepted) {
      try {
        this->accept();
      }
      catch(...) {
        // Do not let exceptions escape the move operation, rather terminate the application instead. This will still
        // print the exception message.
        std::terminate();
      }
    }
    this->accepted = other.accepted;
    this->index = other.index;
    this->valid = other.valid;
    this->_owner = other._owner;
    other.valid = false;
    return *this;
  }

  /********************************************************************************************************************/

  inline bool ReadAnyGroup::Notification::accept() {
    if(!this->valid) {
      throw ChimeraTK::logic_error("This notification object is invalid.");
    }
    if(this->accepted) {
      throw ChimeraTK::logic_error("This notification has already been accepted.");
    }
    this->accepted = true;
    try {
      _owner->push_elements[index].getHighLevelImplElement()->_readQueue.pop_wait();
    }
    catch(ChimeraTK::runtime_error&) {
      _owner->push_elements[index].getHighLevelImplElement()->_activeException = std::current_exception();
    }
    catch(boost::thread_interrupted&) {
      _owner->push_elements[index].getHighLevelImplElement()->_activeException = std::current_exception();
    }
    catch(detail::DiscardValueException&) {
      // we must not call postRead() in this case, hence we do not call preRead()
      _owner->_lastOperationIndex = std::numeric_limits<size_t>::max() - 1;
      return false;
    }
    _owner->_lastOperationIndex = index;
    _owner->push_elements[index].getHighLevelImplElement()->postRead(TransferType::read, true);
    return true;
  }

  /********************************************************************************************************************/

  inline TransferElementID ReadAnyGroup::Notification::getId() {
    if(!this->valid) {
      throw ChimeraTK::logic_error("This notification object is invalid.");
    }
    return _owner->push_elements[index].getId();
  }

  /********************************************************************************************************************/

  inline std::size_t ReadAnyGroup::Notification::getIndex() const {
    if(!this->valid) {
      throw ChimeraTK::logic_error("This notification object is invalid.");
    }
    return this->index;
  }

  /********************************************************************************************************************/

  inline TransferElementAbstractor ReadAnyGroup::Notification::getTransferElement() {
    if(!this->valid) {
      throw ChimeraTK::logic_error("This notification object is invalid.");
    }
    return _owner->push_elements[index];
  }

  /********************************************************************************************************************/

  inline bool ReadAnyGroup::Notification::isReady() const {
    return this->valid && !this->accepted;
  }

  /********************************************************************************************************************/

  inline ReadAnyGroup::Notification::Notification(std::size_t index_, ReadAnyGroup* owner)
  : index(index_), valid(true), _owner(owner) {}

  /********************************************************************************************************************/
  /* Implementations of ReadAnyGroup */
  /********************************************************************************************************************/

  inline ReadAnyGroup::ReadAnyGroup() = default;

  /********************************************************************************************************************/

  inline ReadAnyGroup::ReadAnyGroup(std::initializer_list<std::reference_wrapper<TransferElementAbstractor>> list) {
    for(const auto& element : list) add(element);
    finalise();
  }

  /********************************************************************************************************************/

  inline ReadAnyGroup::ReadAnyGroup(std::initializer_list<boost::shared_ptr<TransferElement>> list) {
    for(const auto& element : list) add(element);
    finalise();
  }

  /********************************************************************************************************************/

  template<typename ITERATOR>
  ReadAnyGroup::ReadAnyGroup(ITERATOR first, ITERATOR last) {
    for(ITERATOR it = first; it != last; ++it) add(*it);
    finalise();
  }

  /********************************************************************************************************************/

  inline ReadAnyGroup& ReadAnyGroup::operator=(ReadAnyGroup&& other) noexcept {
    // we need non-default implementation because we have to move pointers to ReadAnyGroup
    this->isFinalised = other.isFinalised;
    this->push_elements = std::move(other.push_elements);
    this->poll_elements = std::move(other.poll_elements);
    this->_lastOperationIndex = other._lastOperationIndex;
    this->notification_queue = std::move(other.notification_queue);
    for(auto& e : push_elements) {
      e.getHighLevelImplElement()->setInReadAnyGroup(this);
    }
    return *this;
  }

  /********************************************************************************************************************/

  inline void ReadAnyGroup::add(TransferElementAbstractor& element) {
    if(isFinalised) {
      throw ChimeraTK::logic_error("ReadAnyGroup has already been finalised, calling "
                                   "add() is no longer allowed.");
    }
    if(!element.isReadable()) {
      throw ChimeraTK::logic_error(
          "Cannot add non-readable accessor for register " + element.getName() + " to ReadAnyGroup.");
    }
    if(element.getReadAnyGroup() == this) {
      return;
    }
    if(element.getReadAnyGroup() != nullptr) {
      throw ChimeraTK::logic_error(element.getName() + " is already in a different ReadAnyGroup");
    }
    if(element.getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
      push_elements.push_back(element);
    }
    else {
      poll_elements.push_back(element);
    }
    // set flag on the accessor that it is now in a ReadAnyGroup:
    // We do this for push-types only, since poll-types technically still allow calling read() without the ReadAnyGroup,
    // although its documentation states that would not be allowed.
    if(element.getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
      element.getHighLevelImplElement()->setInReadAnyGroup(this);
    }
  }

  /********************************************************************************************************************/

  inline void ReadAnyGroup::add(boost::shared_ptr<TransferElement> element) {
    TransferElementAbstractor a(std::move(element));
    add(a);
  }

  /********************************************************************************************************************/

  inline void ReadAnyGroup::finalise() {
    if(isFinalised) {
      throw ChimeraTK::logic_error("ReadAnyGroup has already been finalised, calling "
                                   "finalise() is no longer allowed.");
    }
    std::vector<cppext::future_queue<void>> queueList;
    bool groupEmpty = true;
    for(auto& e : push_elements) {
      queueList.push_back(e.getHighLevelImplElement()->_readQueue);
      groupEmpty = false;
    }
    if(groupEmpty) {
      throw ChimeraTK::logic_error("ReadAnyGroup has no element with AccessMode::wait_for_new_data.");
    }
    notification_queue = cppext::when_any(queueList.begin(), queueList.end());
    isFinalised = true;
  }

  /********************************************************************************************************************/

  inline TransferElementID ReadAnyGroup::readAny() {
    Notification notification;
    do {
      notification = this->waitAny();
    } while(!notification.accept());

    this->processPolled();

    return notification.getId();
  }

  /********************************************************************************************************************/

  inline TransferElementID ReadAnyGroup::readAnyNonBlocking() {
    Notification notification;
    do {
      notification = this->waitAnyNonBlocking();
      if(!notification.isReady()) {
        this->processPolled();
        return {};
      }
    } while(!notification.accept());

    this->processPolled();

    return notification.getId();
  }
  /********************************************************************************************************************/

  inline void ReadAnyGroup::handlePreRead() {
    // preRead() and postRead() must be called in pairs. Hence we call all preReads here before waiting for transfers to
    // finish. postRead() will be called when accepting the notification. We can call preRead() repeatedly on the same
    // element, even if no transfer and call to postRead() have happened. It is just ignored (see Transfer element spec
    // B.5.2). Since this has a performance impact which might be significant on big applications, we try to avoid
    // unnecessary calls anyway.

    // Notice : This has the side effect that decorators can block here, for instance for the setup phase. This is used
    // by ApplicationCore in testable mode.
    if(_lastOperationIndex == std::numeric_limits<size_t>::max()) {
      for(auto& elem : push_elements) {
        elem.getHighLevelImplElement()->preRead(TransferType::read);
      }
    }
    else if(_lastOperationIndex != std::numeric_limits<size_t>::max() - 1) {
      // Note: _lastOperationIndex is set to std::numeric_limits<size_t>::max() - 1 in case a DiscardValueException
      // has been seen, in which case no postRead() is called.
      push_elements[_lastOperationIndex].getHighLevelImplElement()->preRead(TransferType::read);
    }
  }

  /********************************************************************************************************************/

  inline ReadAnyGroup::Notification ReadAnyGroup::waitAny() {
    handlePreRead();

    // Wait for notification
    std::size_t index;
    notification_queue.pop_wait(index);
    // clazy has a false positive warning about index being uninitialised.
    // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
    return {index, this};
  }

  /********************************************************************************************************************/

  inline ReadAnyGroup::Notification ReadAnyGroup::waitAnyNonBlocking() {
  restart_after_discard_value:
    // check if update is available
    if(notification_queue.empty()) {
      // If no notification is present, do not even execute preRead. This is necessary for two reasons:
      // - We always used TransferType::read to avoid mixing TransferType::read and TransferType::readNonBlocking
      //   in the same transfer of the same variable. We can do this even in this non-blocking case, if we already know
      //   that there will be an update read, since there will not be any difference beyond this point.
      // - In ApplicationCore testable mode, the testable mode lock must be released in a preRead before any blocking
      //   read operation. If preRead is called here when no update is available, no preRead will be called in a
      //   possible subsequend blocking readAny(), hence there would be no way to release the testable mode lock in the
      //   right place.
      return {};
    }

    // if update is available, peek into the queue to check whether a DiscardValueException will be read
    auto id = notification_queue.front();
    try {
      // call to empty() necessary before call to front(), to gain ownership of front element
      if(push_elements[id].getHighLevelImplElement()->_readQueue.empty()) {
        // cannot place the call to empty() into the assert, since it must be executed also in release builds
        assert(false);
      }
      push_elements[id].getHighLevelImplElement()->_readQueue.front();
    }
    catch(detail::DiscardValueException&) {
      // Remove discarded transfer from the queues and go back to square one
      notification_queue.pop();
      try {
        push_elements[id].getHighLevelImplElement()->_readQueue.pop();
      }
      catch(detail::DiscardValueException&) {
        goto restart_after_discard_value;
      }
      assert(false); // we must never end up at this point
    }
    catch(ChimeraTK::runtime_error&) {
      // While peeking we found another runtime which is stored in the queue.
      // Don't let it through, but leave it on the queue and continue with the waitAny().
      // It will be handled later.
    }
    catch(boost::thread_interrupted&) {
      // Also suppress the thread_interrupted exception here.
      // It will stay on the queue, hence it's not lost and will be handled later.
    }
    catch(...) {
      std::cout << "Fatal ERROR in ReadAnyGroup: Found unexpected exception on the read queue. Terminating!"
                << std::endl;
      std::terminate();
    }

    // now that we know that an update is available, we can defer to waitAny()
    return waitAny();
  }

  /********************************************************************************************************************/

  inline void ReadAnyGroup::processPolled() {
    // update all poll-type elements in the group
    for(auto& e : poll_elements) {
      if(!e.getAccessModeFlags().has(AccessMode::wait_for_new_data)) e.readLatest();
    }
  }

  /********************************************************************************************************************/

  inline void ReadAnyGroup::readUntil(const TransferElementID& id) {
    while(true) {
      auto read = readAny();
      if(read == id) return;
    }
  }

  /********************************************************************************************************************/

  inline void ReadAnyGroup::readUntil(const TransferElementAbstractor& element) {
    readUntil(element.getId());
  }

  /********************************************************************************************************************/

  inline void ReadAnyGroup::readUntilAll(const std::vector<TransferElementID>& ids) {
    std::map<TransferElementID, bool> found;
    for(const auto& id : ids) {
      found[id] = false; // initialise map so we can tell from the map if a
                         // variable is in the vector
    }
    size_t leftToFind = ids.size();
    while(true) {
      auto read = readAny();
      if(found.count(read) == 0) continue; // variable is not in the vector ids
      if(found[read]) continue;            // variable has been read already
      found[read] = true;
      --leftToFind;
      if(leftToFind == 0) return;
    }
  }

  /********************************************************************************************************************/

  inline void ReadAnyGroup::readUntilAll(const std::vector<TransferElementAbstractor>& elements) {
    std::map<TransferElementID, bool> found;
    for(const auto& elem : elements) {
      found[elem.getId()] = false; // initialise map so we can tell from the map
                                   // if a variable is in the vector
    }
    size_t leftToFind = elements.size();
    while(true) {
      auto read = readAny();
      if(found.count(read) == 0) continue; // variable is not in the vector elements
      if(found[read]) continue;            // variable has been read already
      found[read] = true;
      --leftToFind;
      if(leftToFind == 0) return;
    }
  }

  /********************************************************************************************************************/

} /* namespace ChimeraTK */
