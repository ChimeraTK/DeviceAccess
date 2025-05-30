// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DataConsistencyGroupSimpleMatcher.h"
#include "TransferElementAbstractor.h"

namespace ChimeraTK {

  class ReadAnyGroup;
  class DataConsistencyGroup;
} // namespace ChimeraTK

namespace ChimeraTK::DataConsistencyGroupDetail {

  /********************************************************************************************************************/

  /**
   * Data consistency matching via history of available data.
   */
  class HistorizedMatcher : public MatcherBase {
   public:
    HistorizedMatcher() = default;
    ~HistorizedMatcher() override;

    /**
     * Add a push element. acc will be decorated by replacing its target -> DataConsistencyDecorator(target). For this
     * reason, we do not provide add function with TransferElement.
     */
    void add(TransferElementAbstractor& acc, unsigned histLen);

    void updateCalled(const TransferElementID& transferElementID) { _updateCalled = transferElementID; }

    /// can be used for diagnostics
    [[nodiscard]] const auto& getTargetElements() const { return _targetElements; }

    // functions needed by DataConsistencyDecorator:
    /**
     * To be called from DataConsistencyDecorator.
     * The given transferElement dictates the VersionNumber to match.
     * It returns true if a match with other transferElements of group can be found by looking through their
     * history of values.
     */
    bool checkUpdate(const TransferElementID& transferElementID);
    /// since after DiscardValueException, ReadAnyGroup does not call preRead at following operation,
    /// DataConsistencyDecorator must 'catch up' on preReads by calling this
    void handleMissingPreReads(TransferElementID callerId);
    void handleMissingPostReads(TransferElementID callerId, bool updateBuffer);

    /// swap data of target buffer into history
    void updateHistory(TransferElementID transferElementID);
    /// returns meta data of last match
    void getMatchingInfo(TransferElementID id, VersionNumber& vs, DataValidity& dv);
    /// returns user buffer of last match
    template<typename UserType>
    std::vector<std::vector<UserType>>& getMatchingBuffer(TransferElementID id);
    [[nodiscard]] TransferElementID lastUpdateCall() const { return _updateCalled; }
    [[nodiscard]] VersionNumber lastMatchingVersionNumber() const { return _lastMatchingVersionNumber; }

   protected:
    /// decorate accessor by replacing its target => DataConsistencyDecorator(target), possibly at an inner level
    /// returns the DataConsistencyDecorator
    boost::shared_ptr<TransferElement> decorateAccessor(TransferElementAbstractor& acc);

    /// element must be target, i.e. not DataConsistencyDecorator
    void setupHistory(const TransferElementAbstractor& element, unsigned histLen);

    bool findMatch(TransferElementID transferElementID);

    /// return reference to target's user buffer of transfer element of this group
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

    bool _decoratorsNeedPreRead = false;
    bool _handleMissingPreReadsCalled = false;
    bool _handleMissingPostReadsCalled = false;

    std::map<TransferElementID, TargetElement> _targetElements;
    VersionNumber _lastMatchingVersionNumber{nullptr};
    TransferElementID _updateCalled; // only for checking usage
  };

  /********************************************************************************************************************/

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

  /********************************************************************************************************************/

  template<typename UserType>
  std::vector<std::vector<UserType>>& HistorizedMatcher::getUserBuffer(TransferElementID transferElementID) {
    // note for improvement - we could use ChimeraTK::TemplateUserTypeMap, see e.g. ConfigReader.cc how to use it
    //  this should eliminiate casting and void* pointer

    const auto& impl = _targetElements.at(transferElementID).acc.getHighLevelImplElement();
    auto acc0 = boost::dynamic_pointer_cast<NDRegisterAccessor<UserType>>(impl);
    assert(acc0);
    return acc0->accessChannels();
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK::DataConsistencyGroupDetail
