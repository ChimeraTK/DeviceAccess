// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "HDataConsistencyGroup.h"

#include "DataConsistencyDecorator.h"
#include "ReadAnyGroup.h"
#include "SupportedUserTypes.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  HDataConsistencyGroup::HDataConsistencyGroup(
      std::initializer_list<std::reference_wrapper<TransferElementAbstractor>> list, unsigned histLen) {
    for(TransferElementAbstractor& acc : list) {
      add(acc, histLen);
    }
  }

  /********************************************************************************************************************/

  void HDataConsistencyGroup::decorateAccessor(TransferElementAbstractor& acc) {
    callForType(acc.getValueType(), [&](auto t) {
      using UserType = decltype(t);

      auto accImpl = boost::dynamic_pointer_cast<NDRegisterAccessor<UserType>>(acc.getHighLevelImplElement());
      assert(accImpl);
      // factory function which creates our DataConsistencyDecorator
      auto factoryF = [&](const boost::shared_ptr<NDRegisterAccessor<UserType>>& toBeDecorated) {
        // let's not replace if toBeDecorated is already a DataConsistencyDecorator
        auto alreadyDecorated = boost::dynamic_pointer_cast<DataConsistencyDecorator<UserType>>(toBeDecorated);
        if(alreadyDecorated) {
          return alreadyDecorated;
        }
        return boost::make_shared<DataConsistencyDecorator<UserType>>(toBeDecorated, this);
      };
      // in case accImpl is ApplicationCore's MetaDataPropagatingRegisterDecorator we need to "go inside" and
      // replace its target by our DataConsistencyDecorator
      if(!accImpl->decorateDeepInside(factoryF)) {
        // accImpl is not itself a decorator, so decorateDeepInside did not do anything.
        acc.replace(factoryF(accImpl));
      }
    });
  }

  /********************************************************************************************************************/

  bool HDataConsistencyGroup::update(const TransferElementID& transferElementID) {
    auto it = _targetElements.find(transferElementID);
    if(it == _targetElements.end()) {
      // ignore unkown element
      return false;
    }

    auto& pElem = it->second;
    auto vn = pElem.acc.getVersionNumber();
    if(pElem.histLen > 0 && vn == pElem.versionNumbers[0]) {
      // take special care for duplicate VersionNumbers. We want VersionNumbers only once in history.
      // So in case of a duplicate VersionNumber, we mark the previous historized value as invalid
      pElem.versionNumbers[0] = VersionNumber(nullptr);
    }

    auto lastMatchingVersionNumber = _lastMatchingVersionNumber; // store value before update
    bool consistent = findMatch(transferElementID);

    if(!consistent) {
      // if not consistent, delay call to postRead (called from ReadAnyGroup), by throwing DiscardValueException.
      // Then, ReadAnyGroup leaves out postRead and next preRead.
      // Add to set of decorators needing call to postRead later.
      _decoratorsNeedingPostRead.insert(transferElementID);
      throw detail::DiscardValueException();
    }

    // In special case that we have consistent state with unchanged version number, we do not update other
    // decorator buffers.
    // E.g. assume that all decorators are in consistent state, say v3.
    // Now A gets a new update, v4. This is put into history, and since there is no value yet for matching B,
    // we throw DiscardValueException. Assume then, we get another value for B, with version v3.
    // In this case, decorator(A) must not be swapped again, but decorator(B) should be swapped with target(B)=v3
    if(_lastMatchingVersionNumber > lastMatchingVersionNumber) {
      // To update other user buffers, call postRead on the other involved decorators.
      // Note, in case of an exception thrown by some postRead, it might happen that postRead is
      // called more than once in a row, for the other elements. This is allowed.
      for(TransferElementID id : _decoratorsNeedingPostRead) {
        auto& acc = _pushElements.at(id);
        acc.getHighLevelImplElement()->postRead(TransferType::read, true);
        // just as in ReadAnyGroup, we immediately follow-up with preRead
        acc.getHighLevelImplElement()->preRead(TransferType::read);
      }
      _decoratorsNeedingPostRead.clear();
    }
    // our own postRead will be called by ReadAnyGroup
    return true;
  }

  /********************************************************************************************************************/

  void HDataConsistencyGroup::add(TransferElementAbstractor& acc, unsigned histLen) {
    if(acc.getReadAnyGroup() == nullptr || (_rag != nullptr && acc.getReadAnyGroup() != _rag)) {
      throw ChimeraTK::logic_error("all elements of the HDataConsistencyGroup must point to the same ReadAnyGroup!");
    }
    _rag = acc.getReadAnyGroup();

    // call setupHistory before decorateAccessor because acc must be target of DataConsistencyDecorator
    setupHistory(acc, histLen);

    decorateAccessor(acc);
    // add decorated access to our elements map (key = Id remains unchanged by decoration)
    _pushElements[acc.getId()] = acc;
    // also find the copy of accessor abstractor in ReadAnyGroup and decorate it in there
    for(auto& pe : _rag->push_elements) {
      if(pe.getId() == acc.getId()) {
        pe.replace(acc);
      }
    }
  }

  /********************************************************************************************************************/

  void HDataConsistencyGroup::setupHistory(TransferElementAbstractor& acc, unsigned histLen) {
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
        // TODO discuss - is it relevant to support ChimeraTK::Void?
        unsigned nSamples = buf[0].size(); // note, this may not work with type Void
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

  HDataConsistencyGroup::~HDataConsistencyGroup() {
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

  bool HDataConsistencyGroup::findMatch(TransferElementID transferElementID) {
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

  void HDataConsistencyGroup::updateHistory(TransferElementID transferElementID) {
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

  void HDataConsistencyGroup::getMatchingInfo(TransferElementID id, VersionNumber& vs, DataValidity& dv) {
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

} // namespace ChimeraTK
