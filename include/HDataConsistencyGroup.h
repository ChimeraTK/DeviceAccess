// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "TransferElementAbstractor.h"

namespace ChimeraTK {

  /*
   *  Conceptual discussions
   *  - ControlSystemAdapters, in particular DoocsAdapter, should also support HistorizedDataConsistencyGroup in places
   *    where it now uses DataConsistencyGroup, e.g. for correlating with macropulse number. Otherwise, data loss
   *    which was prevented by using HistorizedDataConsistencyGroup instead of DataConsistencyGroup in application
   * module, can reappear in DoocsAdapter.
   *
   *  - We might merge functionality of HDataConsistencyGroup into DataConsistencyGroup and introduce
   *    MatchingMode::historized there.
   */

  /********************************************************************************************************************/

  class ReadAnyGroup;

  /**
   * Data consistency matching via history of available data.
   */
  class HDataConsistencyGroup {
   public:
    /**
     * Construct HDataConsistencyGroup from a set of TransferElementAbstractors. These will be decorated
     * with DataConsistencyDecorator.
     * Note, constructor for set of TransferElements is not provided since we could not decorate them for the caller.
     */
    HDataConsistencyGroup(
        std::initializer_list<std::reference_wrapper<TransferElementAbstractor>> list, unsigned histLen = 2);
    virtual ~HDataConsistencyGroup();

    /**
     * Add a push element. Same as constructor from list; acc will be decorated.
     */
    void add(TransferElementAbstractor& acc, unsigned histLen);

    // TODO discuss API
    // we want to create a drop-in replacement for DataConsistencyGroup: add some methods
    // Do we need iterator constructor?
    // Is it fine to call update() like in DataConsistencyGroup?
    // It should be fine since it does nothing, but also useless! We could as well define empty function update()...
    //
    // should we define isConsistent()? It would always return true!
    // can we introduce/extend MatchingMode?

    /**
     * Note, differently from DataConsistencyGroup, explicit call to this function is no longer required,
     * since already called by DataConsistencyDecorator.
     * It behaves similar to DataConsistencyGroup::update.
     * The given transferElement dictates the VersionNumber to match.
     * It returns true if a match with other transferElements of group can be found by looking through their
     * history of values.
     * Returns false otherwise, or if the given transferElement does not belong to group.
     */
    bool update(const TransferElementID& transferElementID);

    /// can be used for diagnostics
    [[nodiscard]] const auto& getPushElements() const { return _targetElements; }

    // functions needed by DataConsistencyDecorator:
    void updateHistory(TransferElementID transferElementID);
    void getMatchingInfo(TransferElementID id, VersionNumber& vs, DataValidity& dv);
    template<typename UserType>
    std::vector<std::vector<UserType>>& getMatchingBuffer(TransferElementID id);

   protected:
    /// decorate accessor by replacing its target => DataConsistencyDecorator(target)
    void decorateAccessor(TransferElementAbstractor& acc);

    /// element must be target, i.e. not DataConsistencyDecorator
    void setupHistory(TransferElementAbstractor& element, unsigned histLen);

    bool findMatch(TransferElementID transferElementID);

    /// return reference to user buffer of transfer element of this group
    template<typename UserType>
    std::vector<std::vector<UserType>>& getUserBuffer(TransferElementID transferElementID);

    template<typename UserBufferType>
    std::vector<UserBufferType>* getBufferVector(TransferElementID id) {
      void* buf = _targetElements.at(id).histBuffer;
      return static_cast<std::vector<UserBufferType>*>(buf);
    }

    struct TargetElement {
      /// target of DataConsistencyDecorator
      TransferElementAbstractor acc;
      unsigned histLen = 0;
      void* histBuffer = nullptr;
      const std::type_info& histBufferType;
      std::vector<VersionNumber> versionNumbers;
      std::vector<DataValidity> dataValidities;
      /// match indices set by findMatch() in case it returns true.
      /// index=0 is most recent value = accessor's user buffer, index>=1 is history buffers
      unsigned lastMatchingIndex = 0;
    };

    std::set<TransferElementID> _decoratorsNeedingPostRead;
    std::map<TransferElementID, TransferElementAbstractor> _pushElements;

    ReadAnyGroup* _rag = nullptr;
    std::map<TransferElementID, TargetElement> _targetElements;
    VersionNumber _lastMatchingVersionNumber{nullptr};
  };

  /*************************************************************************************************/

  template<typename UserType>
  std::vector<std::vector<UserType>>& HDataConsistencyGroup::getMatchingBuffer(TransferElementID id) {
    TargetElement& pe = _targetElements.at(id);
    using UserBufferType = std::vector<std::vector<UserType>>;

    if(pe.lastMatchingIndex > 0) {
      auto& bufferVector = *(getBufferVector<UserBufferType>(id));
      return bufferVector[pe.lastMatchingIndex - 1];
    }
    return getUserBuffer<UserType>(id);
  }

  /*************************************************************************************************/

  template<typename UserType>
  std::vector<std::vector<UserType>>& HDataConsistencyGroup::getUserBuffer(TransferElementID transferElementID) {
    // TODO try using ChimeraTK::TemplateUserTypeMapNoVoid, see e.g. ConfigReader.cc how to use it; this should
    // eliminiate casting (also where I store void* pointer)

    const auto& impl = _targetElements.at(transferElementID).acc.getHighLevelImplElement();
    auto acc0 = boost::dynamic_pointer_cast<NDRegisterAccessor<UserType>>(impl);
    assert(acc0);
    return acc0->accessChannels();
  }

  /*************************************************************************************************/

} // namespace ChimeraTK
