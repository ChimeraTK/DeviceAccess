// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DataConsistencyGroupHistorizedMatcher.h"

#include "DataConsistencyDecorator.h"
#include "ReadAnyGroup.h"
#include "SupportedUserTypes.h"

namespace ChimeraTK::DataConsistencyGroupDetail {

  /********************************************************************************************************************/

  boost::shared_ptr<TransferElement> HistorizedMatcher::decorateAccessor(TransferElementAbstractor& acc) {
    boost::shared_ptr<TransferElement> dataConsistencyDecorator;
    callForType(acc.getValueType(), [&](auto t) {
      using UserType = decltype(t);

      // check if accessor already is in another DataConsistencyGroup
      for(auto& e : acc.getInternalElements()) {
        auto dec = boost::dynamic_pointer_cast<DataConsistencyDecorator<UserType>>(e);
        if(dec) {
          throw ChimeraTK::logic_error(
              "accessor is already in historized DataConsistencyGroup, cannot add it to another one: " + acc.getName());
        }
      }
      auto accImpl = boost::dynamic_pointer_cast<NDRegisterAccessor<UserType>>(acc.getHighLevelImplElement());
      assert(accImpl);
      // factory function which creates our DataConsistencyDecorator
      auto factoryF = [&](const boost::shared_ptr<NDRegisterAccessor<UserType>>& toBeDecorated) {
        return boost::make_shared<DataConsistencyDecorator<UserType>>(toBeDecorated, this);
      };
      // in case accImpl is ApplicationCore's MetaDataPropagatingRegisterDecorator we need to "go inside" and
      // replace its target by our DataConsistencyDecorator
      dataConsistencyDecorator = accImpl->decorateDeepInside(factoryF);
      if(!dataConsistencyDecorator) {
        // accImpl is not itself a decorator, so decorateDeepInside did not do anything.
        dataConsistencyDecorator = factoryF(accImpl);
        acc.replace(dataConsistencyDecorator);
      }
    });
    return dataConsistencyDecorator;
  }

  /********************************************************************************************************************/

  bool HistorizedMatcher::checkUpdate(const TransferElementID& transferElementID) {
    auto it = _targetElements.find(transferElementID);
    assert(it != _targetElements.end());

    auto& pElem = it->second;
    auto vn = pElem.acc.getVersionNumber();
    if(pElem.histLen > 0 && vn == pElem.versionNumbers[0]) {
      // take special care for duplicate VersionNumbers. We want VersionNumbers only once in history.
      // So in case of a duplicate VersionNumber, we mark the previous historized value as invalid
      pElem.versionNumbers[0] = VersionNumber(nullptr);
    }

    bool consistent = findMatch(transferElementID);
    return consistent;
  }

  /********************************************************************************************************************/

  void HistorizedMatcher::handleMissingPreReads(TransferElementID callerId) {
    // prevent recursion by setting flag
    if(_handleMissingPreReadsCalled) {
      return;
    }
    _handleMissingPreReadsCalled = true;
    auto resetFlag = cppext::finally([this] { _handleMissingPreReadsCalled = false; });

    // just a check for right usage: check that DataConsistencyGroup::update was called on updates from ReadAnyGroup.
    if(_decoratorsNeedPreRead) {
      // we know there was already a consistent data update handed out from ReadAnyGroup
      if(lastUpdateCall() != callerId) {
        throw ChimeraTK::logic_error("updates from ReadAnyGroup must be processed by DataConsistencyGroup::update");
      }
    }

    for(auto& e : _pushElements) {
      if(e.first != callerId) {
        auto& acc = e.second;
        acc.getHighLevelImplElement()->preRead(TransferType::read);
      }
    }
    _decoratorsNeedPreRead = false;
  }

  /********************************************************************************************************************/

  void HistorizedMatcher::handleMissingPostReads(TransferElementID callerId, bool updateBuffer) {
    // prevent recursion by setting flag
    if(_handleMissingPostReadsCalled) {
      return;
    }
    _handleMissingPostReadsCalled = true;
    auto resetFlag = cppext::finally([this] { _handleMissingPostReadsCalled = false; });

    // To update other user buffers, call postRead on the other involved decorators.
    // This concerns all pushElements, except when a push element was already on right version num and received
    // another update, which can only happen if the new datum has the same version number (handled as special case).
    // Note, in case of an exception thrown by some postRead, it might happen that postRead is
    // called more than once in a row, for the other elements. This is allowed.
    for(auto& e : _pushElements) {
      if(e.first != callerId) {
        auto& acc = e.second;
        acc.getHighLevelImplElement()->postRead(TransferType::read, updateBuffer);
      }
    }
    _decoratorsNeedPreRead = true;
  }

  /********************************************************************************************************************/

  void HistorizedMatcher::add(TransferElementAbstractor& acc, unsigned histLen) {
    auto* rag = acc.getReadAnyGroup();
    auto id = acc.getId();

    auto dataConsistencyDecorator = decorateAccessor(acc);
    auto target = dataConsistencyDecorator->getInternalElements().front();
    setupHistory(TransferElementAbstractor{target}, histLen);
    // add decorated access to our elements map (key = Id remains unchanged by decoration)
    _pushElements[id] = acc;
    if(rag) {
      // also find the copy of accessor abstractor in ReadAnyGroup and decorate it in there
      for(auto& pe : rag->push_elements) {
        if(pe.getId() == id) {
          pe.replace(acc);
        }
      }
    }
  }

  /********************************************************************************************************************/

  void HistorizedMatcher::setupHistory(const TransferElementAbstractor& acc, unsigned histLen) {
    TransferElementID id = acc.getId();
    if(_targetElements.find(id) != _targetElements.end()) {
      // was alread set up
      return;
    }

    callForType(acc.getValueType(), [&](auto argForType) {
      // set up ring buffer for element's user type
      using UserType = decltype(argForType);
      using UserBufferType = std::vector<std::vector<UserType>>;
      // prepare and insert PushElement not yet having memory (because getUserBuffer requires registered accessor)
      TargetElement element0 = {acc, histLen, nullptr, typeid(UserType), {}, {}};
      _targetElements.insert({id, element0});
      if(histLen > 0) {
        auto* mem = new std::vector<UserBufferType>(histLen);
        // get user buffer just to find out it's shape
        auto& buf = getUserBuffer<UserType>(id);
        unsigned nChannels = buf.size();
        assert(nChannels > 0);
        unsigned nSamples = buf[0].size();
        for(UserBufferType& historyElement : *mem) {
          historyElement.resize(nChannels);
          for(auto& historyElementChannel : historyElement) {
            historyElementChannel.resize(nSamples);
          }
        }
        // continue setup: make buffer known
        TargetElement& element = _targetElements.at(id);
        element.histBuffer = mem;
        element.versionNumbers.resize(histLen);
        std::fill(element.versionNumbers.begin(), element.versionNumbers.end(), VersionNumber{nullptr});
        element.dataValidities.resize(histLen);
      }
    });
  }

  /********************************************************************************************************************/

  HistorizedMatcher::~HistorizedMatcher() {
    for(auto& x : _targetElements) {
      TransferElementID id = x.first;
      TargetElement& element = x.second;
      if(element.histLen > 0) {
        try {
          callForType(element.histBufferType, [&](auto arg) {
            using UserType = decltype(arg);
            using UserBufferType = std::vector<std::vector<UserType>>;
            delete getBufferVector<UserBufferType>(id);
          });
        }
        catch(std::bad_cast& e) {
          // catch + assert in order to satisfy linter
          assert(false);
        }
      }
    }
  }

  /********************************************************************************************************************/

  bool HistorizedMatcher::findMatch(TransferElementID transferElementID) {
    auto it = _targetElements.find(transferElementID);
    if(it == _targetElements.end()) {
      // ignore unknown transfer elements
      return false;
    }
    TargetElement& theElement = it->second;
    auto vn = theElement.acc.getVersionNumber();

    for(auto& pair : _targetElements) {
      if(pair.first == transferElementID) {
        continue;
      }
      TargetElement& element = pair.second;
      // first consider accessor's user buffer and version number
      if(element.acc.getVersionNumber() == vn) {
        element.lastMatchingIndex = 0;
      }
      else if(element.histLen > 0) {
        auto versionNumVector = element.versionNumbers;

        auto pos = std::find(versionNumVector.begin(), versionNumVector.end(), vn);
        if(pos == versionNumVector.end()) {
          return false;
        }
        element.lastMatchingIndex = pos - versionNumVector.begin() + 1;
      }
      else {
        // no direct match and no history
        return false;
      }
    }
    theElement.lastMatchingIndex = 0;
    _lastMatchingVersionNumber = vn;
    return true;
  }

  /********************************************************************************************************************/

  void HistorizedMatcher::updateHistory(TransferElementID transferElementID) {
    TargetElement& element = _targetElements.at(transferElementID);
    if(element.histLen == 0) {
      // exit early if history not availabe for this element
      return;
    }

    VersionNumber vn = element.acc.getVersionNumber();
    DataValidity dv = element.acc.dataValidity();

    callForType(element.histBufferType, [&](auto arg) {
      using UserType = decltype(arg);
      using UserBufferType = std::vector<std::vector<UserType>>;
      auto& buf = getUserBuffer<UserType>(transferElementID);
      // update history of currentBunchPattern so we can use it in findBunchPattern()

      auto& bufferVector = *getBufferVector<UserBufferType>(transferElementID);
      unsigned histLen = bufferVector.size();
      auto& versionNumVector = element.versionNumbers;
      auto& datavalidityVector = element.dataValidities;

      // swap data into history
      // after all swaps, to be erased data is in user buffer. Usually this is the oldest data element,
      // except in special case where first history element is invalid.
      if(versionNumVector[0] > VersionNumber(nullptr)) {
        for(unsigned i = histLen - 1; i > 0; i--) {
          bufferVector[i].swap(bufferVector[i - 1]);
          versionNumVector[i] = versionNumVector[i - 1];
          datavalidityVector[i] = datavalidityVector[i - 1];
        }
      }
      bufferVector[0].swap(buf);
      versionNumVector[0] = vn;
      datavalidityVector[0] = dv;
    });
  }

  /********************************************************************************************************************/

  void HistorizedMatcher::getMatchingInfo(TransferElementID id, VersionNumber& vs, DataValidity& dv) {
    TargetElement& pe = _targetElements.at(id);

    if(pe.lastMatchingIndex > 0) {
      vs = pe.versionNumbers[pe.lastMatchingIndex - 1];
      dv = pe.dataValidities[pe.lastMatchingIndex - 1];
      return;
    }
    vs = pe.acc.getVersionNumber();
    dv = pe.acc.dataValidity();
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK::DataConsistencyGroupDetail
