/*
 * LNMBackendBufferingVariableAccessor.h - Access a variable or constant in a
 * logical name mapping file with a buffering-type accessor.
 *
 *  Created on: Feb 16, 2016
 *      Author: Martin Hierholzer
 */

#pragma once

#  include <algorithm>

#  include "BufferingRegisterAccessor.h"
#  include "Device.h"
#  include "FixedPointConverter.h"
#  include "LogicalNameMappingBackend.h"
#  include "NDRegisterAccessor.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<typename UserType>
  class LNMBackendVariableAccessor : public NDRegisterAccessor<UserType> {
   public:
    LNMBackendVariableAccessor(boost::shared_ptr<DeviceBackend> dev, const RegisterPath& registerPathName,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

    ~LNMBackendVariableAccessor() override;

    void doReadTransferSynchronously() override;

    void doPreWrite(TransferType type, VersionNumber) override;

    bool doWriteTransfer(ChimeraTK::VersionNumber) override;

    bool mayReplaceOther(const boost::shared_ptr<TransferElement const>&) const override;

    bool isReadOnly() const override;

    bool isReadable() const override;

    bool isWriteable() const override;

    void doPreRead(TransferType) override;

    void doPostRead(TransferType, bool hasNewData) override;

    void interrupt() override;

   protected:
    /// register and module name
    RegisterPath _registerPathName;

    /// backend device
    boost::shared_ptr<LogicalNameMappingBackend> _dev;

    /// register information. We have a shared pointer to the original RegisterInfo inside the map, since we need to
    /// modify the value in it (in case of a writeable variable register)
    boost::shared_ptr<LNMBackendRegisterInfo> _info;

    /// Word offset when reading
    size_t _wordOffsetInRegister;

    /// Intermediate buffer used when receiving value from queue, as writing to application buffer must only happen
    /// in doPostRead(). Only used when wait_for_new_data is set.
    typename LNMBackendRegisterInfo::ValueTable<UserType>::QueuedValue _queueValue;

    /// Version number of the last transfer
    VersionNumber currentVersion{nullptr};

    /// access mode flags
    AccessModeFlags _flags;

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
      return {boost::enable_shared_from_this<TransferElement>::shared_from_this()};
    }

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override { return {}; }

    void replaceTransferElement(boost::shared_ptr<TransferElement> /*newElement*/) override {} // LCOV_EXCL_LINE
  };

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(LNMBackendVariableAccessor);

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  template<typename UserType>
  LNMBackendVariableAccessor<UserType>::LNMBackendVariableAccessor(boost::shared_ptr<DeviceBackend> dev,
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags)
  : NDRegisterAccessor<UserType>(registerPathName, flags), _registerPathName(registerPathName),
    _wordOffsetInRegister(wordOffsetInRegister), _flags(flags) {
    // cast device
    _dev = boost::dynamic_pointer_cast<LogicalNameMappingBackend>(dev);

    // obtain the register info
    _info =
        boost::static_pointer_cast<LNMBackendRegisterInfo>(_dev->getRegisterCatalogue().getRegister(_registerPathName));

    // check for unknown flags
    if(_info->targetType == LNMBackendRegisterInfo::TargetType::VARIABLE) {
      flags.checkForUnknownFlags({AccessMode::wait_for_new_data});
    }
    else {
      // no flags are supported for constants
      flags.checkForUnknownFlags({});
    }

    // numberOfWords default to full register length
    if(numberOfWords == 0) numberOfWords = _info->length;

    // check for illegal parameter combinations
    if(wordOffsetInRegister + numberOfWords > _info->length) {
      throw ChimeraTK::logic_error(
          "Requested number of words and/or offset exceeds length of register '" + registerPathName + "'.");
    }

    // check for incorrect usage of this accessor
    if(_info->targetType != LNMBackendRegisterInfo::TargetType::CONSTANT &&
        _info->targetType != LNMBackendRegisterInfo::TargetType::VARIABLE) {
      throw ChimeraTK::logic_error(
          "LNMBackendVariableAccessor used for wrong register type."); // LCOV_EXCL_LINE (impossible to test...)
    }

    // if wait_for_new_data is specified, make subscription
    if(flags.has(AccessMode::wait_for_new_data)) {
      // allocate _queueValue buffer
      this->_queueValue.value.resize(numberOfWords);

      std::lock_guard<std::mutex> lock(_info->valueTable_mutex);
      callForType(_info->valueType, [&, this](auto arg) {
        typedef decltype(arg) T;
        auto& vtEntry = boost::fusion::at_key<T>(_info->valueTable.table);
        // create subscription queue
        cppext::future_queue<typename LNMBackendRegisterInfo::ValueTable<T>::QueuedValue> queue(3);
        // place queue in map
        vtEntry.subscriptions[this->getId()] = queue;
        // make void-typed continuationof subscription queue, which stores the received value into the _queueValue
        this->_readQueue = queue.template then<void>(
            [this](const typename LNMBackendRegisterInfo::ValueTable<T>::QueuedValue& queueValue) {
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

    // allocate application buffer
    NDRegisterAccessor<UserType>::buffer_2D.resize(1);
    NDRegisterAccessor<UserType>::buffer_2D[0].resize(numberOfWords);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  LNMBackendVariableAccessor<UserType>::~LNMBackendVariableAccessor() {
    if(_flags.has(AccessMode::wait_for_new_data)) {
      // unsubscribe the update queue
      std::lock_guard<std::mutex> lock(_info->valueTable_mutex);
      callForType(_info->valueType, [&, this](auto arg) {
        typedef decltype(arg) T;
        auto vtEntry = boost::fusion::at_key<T>(_info->valueTable.table);
        vtEntry.subscriptions.erase(this->getId());
      });
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void LNMBackendVariableAccessor<UserType>::doReadTransferSynchronously() {
    if(_dev->_hasException) {
      throw ChimeraTK::runtime_error("previous, unrecovered fault");
    }
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
    if(_dev->_hasException) {
      throw ChimeraTK::runtime_error("previous, unrecovered fault");
    }
    std::lock_guard<std::mutex> lock(_info->valueTable_mutex);
    callForType(_info->valueType, [&, this](auto arg) {
      auto& vtEntry = boost::fusion::at_key<decltype(arg)>(_info->valueTable.table);

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
  bool LNMBackendVariableAccessor<UserType>::mayReplaceOther(const boost::shared_ptr<TransferElement const>&) const {
    return false; // never replace, since it does not optimise anything
  }

  /********************************************************************************************************************/

  template<typename UserType>
  bool LNMBackendVariableAccessor<UserType>::isReadOnly() const {
    return _info->targetType == LNMBackendRegisterInfo::TargetType::CONSTANT;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  bool LNMBackendVariableAccessor<UserType>::isReadable() const {
    return true;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  bool LNMBackendVariableAccessor<UserType>::isWriteable() const {
    return _info->targetType != LNMBackendRegisterInfo::TargetType::CONSTANT;
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
      std::lock_guard<std::mutex> lock(_info->valueTable_mutex);
      callForType(_info->valueType, [&, this](auto arg) {
        auto& vtEntry = boost::fusion::at_key<decltype(arg)>(_info->valueTable.table);
        for(size_t i = 0; i < this->buffer_2D[0].size(); ++i) {
          this->buffer_2D[0][i] = userTypeToUserType<UserType>(vtEntry.latestValue[i + _wordOffsetInRegister]);
          this->_dataValidity = vtEntry.latestValidity;
        }
      });
      this->_versionNumber = {};
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
    std::lock_guard<std::mutex> lock(_info->valueTable_mutex);
    callForType(_info->valueType, [&, this](auto arg) {
      auto& vtEntry = boost::fusion::at_key<decltype(arg)>(_info->valueTable.table);
      this->interrupt_impl(vtEntry.subscriptions[this->getId()]);
    });
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
