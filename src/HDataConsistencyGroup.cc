// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "HDataConsistencyGroup.h"

#include "DataConsistencyDecorator.h"
#include "ReadAnyGroup.h"
#include "SupportedUserTypes.h"

namespace ChimeraTK {

  HDataConsistencyGroup::HDataConsistencyGroup(
      std::initializer_list<std::reference_wrapper<TransferElementAbstractor>> list) {
    for(TransferElementAbstractor& acc : list) {
      // add target accessor to DataConsistencyGroup
      add(acc);
      decorateAccessor(acc);
      // add decorated access to our elements map (key = Id remains unchanged by decoration)
      decoratedElements[acc.getId()] = acc;
    }
  }

  void HDataConsistencyGroup::decorateAccessor(TransferElementAbstractor& acc) {
    callForType(acc.getValueType(), [&](auto t) {
      using UserType = decltype(t);

      // let's not replace if the target is already a DataConsistencyDecorator
      auto alreadyDecorated =
          boost::dynamic_pointer_cast<DataConsistencyDecorator<UserType>>(acc.getHighLevelImplElement());
      if(!alreadyDecorated) {
        auto accImpl = boost::dynamic_pointer_cast<NDRegisterAccessor<UserType>>(acc.getHighLevelImplElement());
        assert(accImpl);
        acc.replace(boost::make_shared<DataConsistencyDecorator<UserType>>(accImpl, this));
      }
    });
    // TODO decoration magic for MetaDataPropagatingRegisterDecorator:
    // replacement must happen below (i.e. for target of) MetaDataPropagatingRegisterDecorator
  }

  void HDataConsistencyGroup::decorateAccessors(ReadAnyGroup* rag) {
    // replace accessors stored in ReadAnyGroup by our decorated versions, if applicable
    for(auto& acc : rag->push_elements) {
      // id of the target accessor = id of decorator
      auto id = acc.getId();
      if(decoratedElements.find(id) != decoratedElements.end()) {
        acc.replace(decoratedElements.at(id));
      }
    }
  }

  bool HDataConsistencyGroup::update(const TransferElementID& transferElementID) {
    auto lastMatchingVersionNumber = _lastMatchingVersionNumber; // store value before update
    bool consistent = hUpdate(transferElementID);

    if(!consistent) {
      // if not consistent, delay call to postRead (called from ReadAnyGroup), by throwing DiscardValueException.
      // Then, ReadAnyGroup leaves out postRead and next preRead.
      // Add to set of decorators needing call to postRead later.
      decoratorsNeedingPostRead.insert(transferElementID);
      throw detail::DiscardValueException();
    }

    // In special case that we have consistent state with unchanged version number, we do not update other
    // decorator buffers.
    // E.g. all decorators in consistent state, say v3.
    // Now A gets a new update, v4. this is put into history, and since no value yet for matching B,
    // we throw DiscardValueException. But then, we get another value for B, with version v3.
    // In this case, decorator(A) must not be swapped again, but decorator(B) should be swapped with target(B)=v3
    if(_lastMatchingVersionNumber > lastMatchingVersionNumber) {
      // To update other user buffers, call postRead on the other involved decorators.
      // Note, in case of an exception thrown by some postRead, it might happen that postRead is
      // called more than once in a row, for the other elements. This is allowed.
      for(TransferElementID id : decoratorsNeedingPostRead) {
        auto& acc = decoratedElements.at(id);
        acc.getHighLevelImplElement()->postRead(TransferType::read, true);
        // just as in ReadAnyGroup, we immediately follow-up with preRead
        acc.getHighLevelImplElement()->preRead(TransferType::read);
      }
      decoratorsNeedingPostRead.clear();
    }
    // our own postRead will be called by ReadAnyGroup
    return true;
  }

  /********************************************************************************************************************/

  void HDataConsistencyGroup::add(ChimeraTK::TransferElementAbstractor& acc, unsigned histLen) {
    // TODO - either add should not be public or change behavior
    setupHistory(acc, histLen);
  }

  /********************************************************************************************************************/

  void HDataConsistencyGroup::setupHistory(const ChimeraTK::TransferElementAbstractor& acc, unsigned histLen) {
    ChimeraTK::TransferElementID id = acc.getId();
    if(_pushElements.find(id) != _pushElements.end()) {
      // was alread set up
      return;
    }

    ChimeraTK::callForType(acc.getValueType(), [&](auto argForType) {
      // set up ring buffer for element's user type
      using UserType = decltype(argForType);
      using UserBufferType = std::vector<std::vector<UserType>>;
      // prepare and insert PushElement not yet having memory (because getUserBuffer requires registered accessor)
      PushElement element0 = {acc, histLen, nullptr, typeid(UserType), {}, {}};
      _pushElements.insert({id, element0});
      if(histLen > 0) {
        auto* mem = new std::vector<UserBufferType>(histLen);
        // get user buffer just to find out it's shape
        auto& buf = getUserBuffer<UserType>(id);
        unsigned nChannels = buf.size();
        assert(nChannels > 0);
        unsigned nSamples = buf[0].size(); // note, this may not work with type Void
        for(UserBufferType& historyElement : *mem) {
          historyElement.resize(nChannels);
          for(auto& historyElementChannel : historyElement) {
            historyElementChannel.resize(nSamples);
          }
        }
        // continue setup: make buffer known
        PushElement& element = _pushElements.at(id);
        element.histBuffer = mem;
        element.versionNumbers.resize(histLen);
        std::fill(element.versionNumbers.begin(), element.versionNumbers.end(), ChimeraTK::VersionNumber{0});
        element.dataValidities.resize(histLen);
      }
    });
  }

  /********************************************************************************************************************/

  HDataConsistencyGroup::~HDataConsistencyGroup() {
    for(auto& x : _pushElements) {
      ChimeraTK::TransferElementID id = x.first;
      PushElement& element = x.second;
      if(element.histLen > 0) {
        try {
          ChimeraTK::callForType(element.histBufferType, [&](auto arg) {
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

  bool HDataConsistencyGroup::hUpdate(const ChimeraTK::TransferElementID& transferElementID) {
    auto it = _pushElements.find(transferElementID);
    if(it == _pushElements.end()) {
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

    if(findMatch(transferElementID)) {
      // match found, swap back values of all except this transferElement into target(i.e. not decorator) buffers
      for(const auto& pair : _pushElements) {
        if(pair.first == transferElementID) {
          continue;
        }
        updateUserBuffer(pair.first);
      }
      _lastMatchingVersionNumber = vn;
      return true;
    }
    return false;
  }

  /********************************************************************************************************************/

  bool HDataConsistencyGroup::findMatch(TransferElementID transferElementID) {
    auto it = _pushElements.find(transferElementID);
    if(it == _pushElements.end()) {
      // ignore unknown transfer elements
      return false;
    }
    auto vn = it->second.acc.getVersionNumber();

    for(auto& pair : _pushElements) {
      if(pair.first == transferElementID) {
        continue;
      }
      PushElement& element = pair.second;
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
    return true;
  }

  /********************************************************************************************************************/

  void HDataConsistencyGroup::updateUserBuffer(const ChimeraTK::TransferElementID& transferElementID) {
    PushElement& element = _pushElements.at(transferElementID);
    auto f = [&](auto argForType) {
      using UserType = decltype(argForType);
      using UserBufferType = std::vector<std::vector<UserType>>;
      auto& bufferVector = *getBufferVector<UserBufferType>(transferElementID);

      // update user buffer from history
      UserBufferType& userBuffer = getUserBuffer<UserType>(transferElementID);
      UserBufferType& matchingBuffer = bufferVector[element.lastMatchingIndex - 1];
      for(size_t i = 0; i < userBuffer.size(); ++i) {
        userBuffer[i].swap(matchingBuffer[i]);
      }
      auto savedDv = element.dataValidities[element.lastMatchingIndex - 1];
      element.acc.setDataValidity(savedDv);
    };

    // user buffer update applies only lastMatchingIndex points to history
    if(element.lastMatchingIndex > 0) {
      ChimeraTK::callForType(element.histBufferType, f);
    }
  }

  /********************************************************************************************************************/

  void HDataConsistencyGroup::updateHistory(const ChimeraTK::TransferElementID& transferElementID) {
    PushElement& element = _pushElements.at(transferElementID);
    if(element.histLen == 0) {
      // exit early if history not availabe for this element
      return;
    }

    ChimeraTK::VersionNumber vn = element.acc.getVersionNumber();
    ChimeraTK::DataValidity dv = element.acc.dataValidity();

    ChimeraTK::callForType(element.histBufferType, [&](auto arg) {
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

} // namespace ChimeraTK
