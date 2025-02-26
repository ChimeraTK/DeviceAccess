// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "TransferElementAbstractor.h"

namespace ChimeraTK {

  /*
   *
   *  Conceptual discussion of handling of meta data like VersionNumber and DataValidity
   *  see also https://redmine.msktools.desy.de/issues/9079#change-44259
   *
   *  First discussion results
   *  - We want to store the meta data in history and put it back to the user buffers
   *  - In particular, we also want to restore version numbers. This requires that we introduce a new method
   *    setVerionNumber() in TransferElementAbstractor.
   *  - It is also important to re-wind VersionNumber of the ApplicationModule to the one of the consistent data set,
   *    because it is used for VersionNumber of the ApplicatonModule's outputs.
   *    Downstream modules (and also ControlSystemAdapter) take it as input for their input's VersionNumbers.
   *  - ControlSystemAdapters, in particular DoocsAdapter, should also support HistorizedDataConsistencyGroup in places
   *    where it now uses DataConsistencyGroup, e.g. for correlating with macropulse number. Otherwise, data loss
   *    which was prevented by using HistorizedDataConsistencyGroup instead of DataConsistencyGroup in application
   * module, can reappear in DoocsAdapter.
   *
   *  - we should introduce DataConsistencyGroup::getPushElements()
   *  - We might merge functionality of HistorizedDataConsistencyGroup into DataConsistencyGroup and introduce
   *    MatchingMode::historized there.
   *  - DataValidity of the user buffers is already restored from history. However, we also need to restore DataValidity
   *    of the ApplicationModule. This might be implemented by using
   *    ApplicationModule::increment/decrementDataFaultCounter (intermediate negative values?)
   *    If HistorizedDataConsistencyGroup is in DeviceAccess, will need ApplicationCore variant deriving from it.
   *
   */

  /********************************************************************************************************************/

  class ReadAnyGroup;

  /**
   * Data consistency matching via history of available data.
   * Idea is to add sets of accessors. Each set is managed by a usual DataConsistencyGroup.
   * The sets may be supplemented by event sources, each set having exactly one, of the same type so their values can
   * be compared. The template parameter EventId is user type of the event sources.
   * As a special case, not specifying EventId selects EventId = VersionNumber. In this case, no event source must be
   * specified, and VersionNumbers are used also for matching across managed sets.
   */
  class HDataConsistencyGroup {
   public:
    /**
     * Construct HDataConsistencyGroup from a set of TransferElementAbstractors. These will be decorated
     * with DataConsistencyDecorator.
     * Note, constructor for set of TransferElements is not provided since we could not decorate them for the caller.
     */
    HDataConsistencyGroup(std::initializer_list<std::reference_wrapper<TransferElementAbstractor>> list);

    /**
     * Add a push element.
     */
    void add(ChimeraTK::TransferElementAbstractor& acc, unsigned histLen = 2);

    // TODO discuss - should this be public or not?
    /// decorate accessor by replacing its target => DataConsistencyDecorator(target)
    void decorateAccessor(TransferElementAbstractor& acc);
    // TODO discuss - should this be public or not?
    /// use known already decorated accessors as replacement for ReadAnyGroup's content
    void decorateAccessors(ReadAnyGroup* rag);

    /**
     * Note, differently from DataConsistencyGroup, explicit call to this function is no longer required,
     * since already done by DataConsistencyDecorator.
     * behaves similar to DataConsistencyGroup::update.
     * The given transferElement dictates the VersionNumber to match.
     * Returns true if a match with other transferElements of group can be found by looking through their
     * history of values
     * Returns false if given transferElement does not belong to group.
     */
    bool update(const TransferElementID& transferElementID);

    [[nodiscard]] const auto& getPushElements() const { return _pushElements; }
    virtual ~HDataConsistencyGroup();

    void updateHistory(const ChimeraTK::TransferElementID& transferElementID);

   protected:
    std::set<TransferElementID> decoratorsNeedingPostRead;
    std::map<TransferElementID, TransferElementAbstractor> decoratedElements;

    void setupHistory(const ChimeraTK::TransferElementAbstractor& element, unsigned histLen);

    // TODO make mostly compatible as a drop-in replacement for DataConsistencyGroup: add some methods

    bool findMatch(TransferElementID transferElementID);

   public:
    /// return reference to user buffer of transfer element of this group
    template<typename UserType>
    std::vector<std::vector<UserType>>& getUserBuffer(const ChimeraTK::TransferElementID& transferElementID);

    void updateUserBuffer(const ChimeraTK::TransferElementID& transferElementID);

    template<typename UserBufferType>
    std::vector<UserBufferType>* getBufferVector(ChimeraTK::TransferElementID id) {
      void* buf = _pushElements.at(id).histBuffer;
      return static_cast<std::vector<UserBufferType>*>(buf);
    }

   protected:
    struct PushElement {
      // target of DataConsistencyDecorator
      ChimeraTK::TransferElementAbstractor acc;
      unsigned histLen = 0;
      void* histBuffer = nullptr;
      const std::type_info& histBufferType;
      std::vector<ChimeraTK::VersionNumber> versionNumbers;
      std::vector<ChimeraTK::DataValidity> dataValidities;
      /// match indices set by findMatch() in case it returns true.
      /// index=0 is most recent value = accessor's user buffer, index>=1 is history buffers
      unsigned lastMatchingIndex = 0;
    };

    std::map<ChimeraTK::TransferElementID, PushElement> _pushElements;
    VersionNumber _lastMatchingVersionNumber{nullptr};

   public:
    std::map<ChimeraTK::TransferElementID, PushElement>& getPushElements() { return _pushElements; }
  };

  /*************************************************************************************************/

  template<typename UserType>
  std::vector<std::vector<UserType>>& HDataConsistencyGroup::getUserBuffer(
      const ChimeraTK::TransferElementID& transferElementID) {
    // TODO try using ChimeraTK::TemplateUserTypeMapNoVoid, see e.g. ConfigReader.cc how to use it; this should
    // eliminiate casting (also where I store void* pointer)

    const auto& impl = _pushElements.at(transferElementID).acc.getHighLevelImplElement();
    auto acc0 = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<UserType>>(impl);
    assert(acc0);
    return acc0->accessChannels();
  }

  /*************************************************************************************************/

} // namespace ChimeraTK
