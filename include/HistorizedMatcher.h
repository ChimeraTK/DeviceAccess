// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "TransferElementAbstractor.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  class ReadAnyGroup;
  class DataConsistencyGroup;

  /**
   * Data consistency matching via history of available data.
   */
  class HistorizedMatcher {
   public:
    explicit HistorizedMatcher(DataConsistencyGroup* dg) : _dg(dg) {}
    virtual ~HistorizedMatcher();

    /**
     * Add a push element. acc will be decorated by replacing its target -> DataConsistencyDecorator(target). For this
     * reason, we do not provide add function with TransferElement.
     */
    void add(TransferElementAbstractor& acc, unsigned histLen);

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
    [[nodiscard]] const auto& getTargetElements() const { return _targetElements; }

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

    DataConsistencyGroup* _dg;
    ReadAnyGroup* _rag = nullptr;

    std::set<TransferElementID> _decoratorsNeedingPostRead;

    std::map<TransferElementID, TargetElement> _targetElements;
    VersionNumber _lastMatchingVersionNumber{nullptr};
  };

  /*************************************************************************************************/

  template<typename UserType>
  std::vector<std::vector<UserType>>& HistorizedMatcher::getMatchingBuffer(TransferElementID id) {
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
  std::vector<std::vector<UserType>>& HistorizedMatcher::getUserBuffer(TransferElementID transferElementID) {
    // TODO try using ChimeraTK::TemplateUserTypeMapNoVoid, see e.g. ConfigReader.cc how to use it; this should
    // eliminiate casting (also where I store void* pointer)

    const auto& impl = _targetElements.at(transferElementID).acc.getHighLevelImplElement();
    auto acc0 = boost::dynamic_pointer_cast<NDRegisterAccessor<UserType>>(impl);
    assert(acc0);
    return acc0->accessChannels();
  }

  /*************************************************************************************************/

} // namespace ChimeraTK
