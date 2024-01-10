// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LNMBackendVariableAccessor.h"

#include "internal/LNMMathPluginFormulaHelper.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<typename UserType>
  LNMBackendVariableAccessor<UserType>::LNMBackendVariableAccessor(const boost::shared_ptr<DeviceBackend>& dev,
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags)
  : NDRegisterAccessor<UserType>(registerPathName, flags), _registerPathName(registerPathName),
    _wordOffsetInRegister(wordOffsetInRegister), _flags(flags) {
    // cast device
    _dev = boost::dynamic_pointer_cast<LogicalNameMappingBackend>(dev);

    // obtain the register info
    _info = _dev->_catalogue_mutable.getBackendRegister(_registerPathName);
    // boost::static_pointer_cast<BackendRegisterCatalogue<LNMBackendRegisterInfo>>(_dev->getRegisterCatalogue().getRegister(_registerPathName));

    // check for unknown flags
    if(_info.targetType == LNMBackendRegisterInfo::TargetType::VARIABLE) {
      flags.checkForUnknownFlags({AccessMode::wait_for_new_data});
    }
    else {
      // no flags are supported for constants
      flags.checkForUnknownFlags({});
    }

    // numberOfWords default to full register length
    if(numberOfWords == 0) numberOfWords = _info.length;

    // check for illegal parameter combinations
    if(wordOffsetInRegister + numberOfWords > _info.length) {
      throw ChimeraTK::logic_error(
          "Requested number of words and/or offset exceeds length of register '" + registerPathName + "'.");
    }

    // check for incorrect usage of this accessor
    if(_info.targetType != LNMBackendRegisterInfo::TargetType::CONSTANT &&
        _info.targetType != LNMBackendRegisterInfo::TargetType::VARIABLE) {
      throw ChimeraTK::logic_error("LNMBackendVariableAccessor used for wrong register type."); // LCOV_EXCL_LINE
                                                                                                // (impossible to
                                                                                                // test...)
    }

    // if wait_for_new_data is specified, make subscription
    if(flags.has(AccessMode::wait_for_new_data)) {
      // allocate _queueValue buffer
      this->_queueValue.value.resize(numberOfWords);

      auto& lnmVariable = _dev->_variables[_info.name];
      std::lock_guard<std::mutex> lock(lnmVariable.valueTable_mutex);

      callForType(_info.valueType, [&, this](auto arg) {
        using T = decltype(arg);
        auto& vtEntry = boost::fusion::at_key<T>(lnmVariable.valueTable.table);
        // create subscription queue
        cppext::future_queue<typename LNMVariable::ValueTable<T>::QueuedValue> queue(3);
        // place queue in map
        vtEntry.subscriptions[this->getId()] = queue;
        // make void-typed continuation of subscription queue, which stores the received value into the _queueValue
        this->_readQueue = queue.template then<void>(
            [this](const typename LNMVariable::ValueTable<T>::QueuedValue& queueValue) {
              this->_queueValue.validity = queueValue.validity;
              this->_queueValue.version = queueValue.version;
              for(size_t i = 0; i < queueValue.value.size(); ++i) {
                this->_queueValue.value[i] =
                    userTypeToUserType<UserType>(queueValue.value[i + this->_wordOffsetInRegister]);
              }
            },
            std::launch::deferred);
        // put initial value into queue, if async reads activated
        if(_dev->_asyncReadActive) {
          queue.push({vtEntry.latestValue, vtEntry.latestValidity, vtEntry.latestVersion});
        }
      });
    }

    // make sure FormulaHelpers for MathPlugin instances involving this variable as push-parameter are created
    auto& lnmVariable = _dev->_variables[_info.name];
    for(auto* mp : lnmVariable.usingFormulas) {
      if(mp->_hasPushParameter) {
        // following check eliminates recursion, getFormulaHelper also creates Variable accessors
        if(!mp->_creatingFormulaHelper) {
          auto h = mp->getFormulaHelper(_dev);
          _formulaHelpers.push_back(h);
        }
      }
    }

    // allocate application buffer
    NDRegisterAccessor<UserType>::buffer_2D.resize(1);
    NDRegisterAccessor<UserType>::buffer_2D[0].resize(numberOfWords);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  LNMBackendVariableAccessor<UserType>::~LNMBackendVariableAccessor() {
    if(_flags.has(AccessMode::wait_for_new_data)) {
      // unsubscribe the update queue
      auto& lnmVariable = _dev->_variables[_info.name];
      std::lock_guard<std::mutex> lock(lnmVariable.valueTable_mutex);
      callForType(_info.valueType, [&, this](auto arg) {
        using T = decltype(arg);
        auto& vtEntry = boost::fusion::at_key<T>(lnmVariable.valueTable.table);
        vtEntry.subscriptions.erase(this->getId());
      });
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void LNMBackendVariableAccessor<UserType>::doReadTransferSynchronously() {
    _dev->checkActiveException();
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void LNMBackendVariableAccessor<UserType>::doPreWrite(TransferType type, VersionNumber) {
    std::ignore = type;
    if(!LNMBackendVariableAccessor<UserType>::isWriteable()) {
      throw ChimeraTK::logic_error(
          "Writing to constant-type registers of logical name mapping devices is not possible.");
    }
    if(!_dev->_opened) { // directly use member variables as friend to avoid virtual function calls
      throw ChimeraTK::logic_error("Cannot write to a closed device.");
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  bool LNMBackendVariableAccessor<UserType>::doWriteTransfer(ChimeraTK::VersionNumber v) {
    _dev->checkActiveException();
    auto& lnmVariable = _dev->_variables[_info.name];
    std::lock_guard<std::mutex> lock(lnmVariable.valueTable_mutex);

    callForType(_info.valueType, [&, this](auto arg) {
      auto& vtEntry = boost::fusion::at_key<decltype(arg)>(lnmVariable.valueTable.table);

      // store new value as latest value
      for(size_t i = 0; i < this->buffer_2D[0].size(); ++i) {
        vtEntry.latestValue[i + _wordOffsetInRegister] = userTypeToUserType<decltype(arg)>(this->buffer_2D[0][i]);
      }
      vtEntry.latestValidity = this->dataValidity();
      vtEntry.latestVersion = v;

      // push new value to subscription queues, if async read is activated
      if(_dev->_asyncReadActive) {
        for(auto& sub : vtEntry.subscriptions) {
          sub.second.push_overwrite({vtEntry.latestValue, this->dataValidity(), v});
        }
      }
    });
    return false;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void LNMBackendVariableAccessor<UserType>::doPostWrite(
      TransferType /*type*/, ChimeraTK::VersionNumber versionNumber) {
    // call write functions which make use of this parameter in MathPlugin-handled formulas
    for(const auto& h : _formulaHelpers) {
      h->updateResult(versionNumber);
      // error handling: updateResult does it already.
      // we don't want to issue exceptions from VariableAccessor, since a variable change is not closely related
      // to where the error appears (e.g. error appears when writing to target)
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  bool LNMBackendVariableAccessor<UserType>::mayReplaceOther(const boost::shared_ptr<TransferElement const>&) const {
    return false; // never replace, since it does not optimise anything
  }

  /********************************************************************************************************************/

  template<typename UserType>
  bool LNMBackendVariableAccessor<UserType>::isReadOnly() const {
    return _info.targetType == LNMBackendRegisterInfo::TargetType::CONSTANT;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  bool LNMBackendVariableAccessor<UserType>::isReadable() const {
    return true;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  bool LNMBackendVariableAccessor<UserType>::isWriteable() const {
    return _info.targetType != LNMBackendRegisterInfo::TargetType::CONSTANT;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void LNMBackendVariableAccessor<UserType>::doPreRead(TransferType) {
    if(!_dev->_opened) {
      throw ChimeraTK::logic_error("Cannot read from a closed device.");
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void LNMBackendVariableAccessor<UserType>::doPostRead(TransferType, bool hasNewData) {
    if(!hasNewData) return;

    if(!_flags.has(AccessMode::wait_for_new_data)) {
      // poll-type read transfer: fetch latest value from ValueTable
      auto& lnmVariable = _dev->_variables[_info.name];
      std::lock_guard<std::mutex> lock(lnmVariable.valueTable_mutex);

      callForType(_info.valueType, [&, this](auto arg) {
        auto& vtEntry = boost::fusion::at_key<decltype(arg)>(lnmVariable.valueTable.table);
        for(size_t i = 0; i < this->buffer_2D[0].size(); ++i) {
          this->buffer_2D[0][i] = userTypeToUserType<UserType>(vtEntry.latestValue[i + _wordOffsetInRegister]);
        }
        this->_dataValidity = vtEntry.latestValidity;
        // Note: passing through the version number also for push-type variables is essential for the MathPlugin (cf.
        // MathPluginFormulaHelper::checkAllParametersWritten()) and does not violate the spec (spec says we should not
        // be able to see whether there was an update, which still is impossible since updates can have the same
        // version number as before).
        this->_versionNumber = vtEntry.latestVersion;
      });
    }
    else {
      // push-type read transfer: received value is in _queueValue (cf. readQueue continuation in constructor)
      this->buffer_2D[0].swap(_queueValue.value);
      this->_versionNumber = _queueValue.version;
      this->_dataValidity = _queueValue.validity;
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void LNMBackendVariableAccessor<UserType>::interrupt() {
    auto& lnmVariable = _dev->_variables[_info.name];
    std::lock_guard<std::mutex> lock(lnmVariable.valueTable_mutex);

    callForType(_info.valueType, [&, this](auto arg) {
      auto& vtEntry = boost::fusion::at_key<decltype(arg)>(lnmVariable.valueTable.table);
      this->interrupt_impl(vtEntry.subscriptions[this->getId()]);
    });
  }

  /********************************************************************************************************************/

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(LNMBackendVariableAccessor);
} // namespace ChimeraTK
