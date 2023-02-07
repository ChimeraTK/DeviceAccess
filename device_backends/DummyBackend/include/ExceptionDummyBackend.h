// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DummyBackend.h"
#include "NDRegisterAccessorDecorator.h"

#include <utility>

namespace ChimeraTK {

  template<typename UserType>
  struct ExceptionDummyPushDecorator;

  struct ExceptionDummyPushDecoratorBase;

  struct ExceptionDummyPollDecoratorBase;
  template<typename UserType>
  struct ExceptionDummyPollDecorator;

  /********************************************************************************************************************/

  class ExceptionDummy : public ChimeraTK::DummyBackend {
   public:
    explicit ExceptionDummy(std::string const& mapFileName) : DummyBackend(mapFileName) {
      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
    }

    std::atomic<bool> throwExceptionOpen{false};
    std::atomic<bool> throwExceptionRead{false};
    std::atomic<bool> throwExceptionWrite{false};
    std::atomic<bool> thereHaveBeenExceptions{false};

    static boost::shared_ptr<DeviceBackend> createInstance(
        [[maybe_unused]] std::string, std::map<std::string, std::string> parameters) {
      return boost::shared_ptr<DeviceBackend>(new ExceptionDummy(parameters["map"]));
    }

    void open() override {
      if(throwExceptionOpen) {
        thereHaveBeenExceptions = true;
        throw(ChimeraTK::runtime_error("DummyException: open throws by request"));
      }
      ChimeraTK::DummyBackend::open();
      thereHaveBeenExceptions = false;
    }

    void closeImpl() override {
      setException();
      DummyBackend::closeImpl();
    }

    void read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) override {
      if(throwExceptionRead) {
        thereHaveBeenExceptions = true;
        throw(ChimeraTK::runtime_error("DummyException: read throws by request"));
      }
      ChimeraTK::DummyBackend::read(bar, address, data, sizeInBytes);
    }

    void write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) override {
      if(throwExceptionWrite) {
        thereHaveBeenExceptions = true;
        throw(ChimeraTK::runtime_error("DummyException: write throws by request"));
      }
      ChimeraTK::DummyBackend::write(bar, address, data, sizeInBytes);

      // increment write counter and update write order (only if address points to beginning of a register!)
      auto itWriteOrder = _writeOrderMap.find(std::make_pair(bar, address));
      if(itWriteOrder != _writeOrderMap.end()) {
        // update write order
        auto generatedOrderNumber = ++_writeOrderCounter;
        auto& orderNumberInMap = itWriteOrder->second;
        // atomically update order number in the map only if the generated order number is bigger. This will be always
        // the case, unless there is a concurrent write operation updating the order number in between.
        size_t current;
        while((current = orderNumberInMap.load()) < generatedOrderNumber) {
          orderNumberInMap.compare_exchange_weak(current, generatedOrderNumber);
        }

        // increment write counter
        auto itWriteCounter = _writeCounterMap.find(std::make_pair(bar, address));
        assert(itWriteCounter != _writeCounterMap.end()); // always inserted together
        itWriteCounter->second++;
      }
    }

    bool isFunctional() const override {
      // thereHaveBeenExceptions is different from _hasActiceException
      // * thereHaveBeenExceptions is set when this class originally raised an exception
      // * _hasActiveException is raised externally via setException. This can happen if a transfer element from another
      // backend,
      //   which is in the same logical name mapping backend than this class, has seen an exception.
      return (_opened && !throwExceptionOpen && !thereHaveBeenExceptions && !_hasActiveException);
    }

    /// Specific override which allows to create push-type accessors
    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(const RegisterPath& registerPathName,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
      auto path = registerPathName;
      path.setAltSeparator(".");
      auto pathComponents = path.getComponents();
      bool pushRead = false;
      if(pathComponents[pathComponents.size() - 1] == "PUSH_READ") {
        if(flags.has(AccessMode::wait_for_new_data)) {
          pushRead = true;
          flags.remove(AccessMode::wait_for_new_data);
        }
        path--; // remove last component
      }

      auto acc = DummyBackendBase::getRegisterAccessor_impl<UserType>(path, numberOfWords, wordOffsetInRegister, flags);
      if(pushRead) {
        std::unique_lock<std::mutex> lk(_pushDecoratorsMutex);

        auto decorator = boost::make_shared<ExceptionDummyPushDecorator<UserType>>(
            acc, boost::dynamic_pointer_cast<ExceptionDummy>(this->shared_from_this()));

        _pushDecorators[registerPathName].push_back(decorator);

        if(_activateNewPushAccessors) {
          decorator->_isActive = true;
          decorator->trigger(); // initial value
        }

        acc = decorator;
      }
      else {
        // decorate all poll-type variable so returned validity of the data can be controlled
        auto decorator = boost::make_shared<ExceptionDummyPollDecorator<UserType>>(
            acc, boost::dynamic_pointer_cast<ExceptionDummy>(this->shared_from_this()));
        acc = decorator;
      }

      // create entry in _writeOrderMap and _writeCounterMap if necessary
      if(pathComponents[pathComponents.size() - 1] != "DUMMY_WRITEABLE" &&
          (pathComponents[0].find("DUMMY_INTERRUPT_") != 0)) {
        auto info = getRegisterInfo(path);
        auto adrPair = std::make_pair(info.bar, info.address);
        if(_writeOrderMap.find(adrPair) == _writeOrderMap.end()) {
          _writeOrderMap[adrPair] = 0;
          _writeCounterMap[adrPair] = 0;
        }
      }

      return acc;
    }

    /// Function to trigger sending values for push-type variables
    void triggerPush(RegisterPath path, VersionNumber v = {});

    /// Function to obtain the write order number of a register. Comparing the write order number for different
    /// registers allows to determine which register has been written last (later writs have bigger write order
    /// numbers).
    /// Note: This currently only works if writes are always starting at the beginning of the register (i.e. without an
    /// offset relative to the register). Also does not work for DUMMY_WRITEABLE registers.
    size_t getWriteOrder(const RegisterPath& path);

    /// Function to obtain the number of writes of a register since the creation of the backend.
    /// Note: This currently only works if writes are always starting at the beginning of the register (i.e. without an
    /// offset relative to the register). Also does not work for DUMMY_WRITEABLE registers.
    size_t getWriteCount(const RegisterPath& path);

    void activateAsyncRead() noexcept override;

    void setException() override;

    /// Function to test whether async read transfers are activated
    bool asyncReadActivated();

    /// Mutex to protect data structures for push decorators
    std::mutex _pushDecoratorsMutex;
    /// Map of active ExceptionDummyPushDecorator. Protected by _pushDecoratorMutex
    std::map<RegisterPath, std::list<boost::weak_ptr<ExceptionDummyPushDecoratorBase>>> _pushDecorators;
    /// Map of version numbers to use in push decorators. Protected by _pushDecoratorMutex
    std::map<RegisterPath, VersionNumber> _pushVersions;
    /// Flag is toggled by activateAsyncRead (true), setException (false) and close (false).
    /// Protected by _pushDecoratorMutex
    bool _activateNewPushAccessors{false};

    /// mutex to protect map _registerValidities
    std::mutex _registerValiditiesMutex;
    /// This map is used for setting individual (poll-type) variables to DataValidity=faulty
    std::map<RegisterPath, DataValidity> _registerValidities;
    /// Use decorator to overwrite returned data validity of individual (poll-type) variables.
    /// Works only in the direction valid->invalid.
    void setValidity(RegisterPath path, DataValidity val) {
      path.setAltSeparator(".");
      std::unique_lock<std::mutex> lk(_registerValiditiesMutex);
      _registerValidities[path] = val;
    }
    /// Query map for overwritten data validities.
    DataValidity getValidity(RegisterPath path) {
      path.setAltSeparator(".");
      std::unique_lock<std::mutex> lk(_registerValiditiesMutex);
      // simply return default enum value 0 = DataValidity::ok unless path is explicity found in map
      return _registerValidities[path];
    }

    /// Map used allow determining order of writes by tests. Map key is pair of bar and address.
    std::map<std::pair<uint64_t, uint64_t>, std::atomic<size_t>> _writeOrderMap;

    /// Global counter for order numbers going into _writeOrderMap
    std::atomic<size_t> _writeOrderCounter{0};

    /// Map used allow determining number of writes of a specific register by tests. Map key is pair of bar and address.
    std::map<std::pair<uint64_t, uint64_t>, std::atomic<size_t>> _writeCounterMap;

    class BackendRegisterer {
     public:
      BackendRegisterer();
    };
    static BackendRegisterer backendRegisterer;
  };

  /********************************************************************************************************************/

  struct ExceptionDummyPushDecoratorBase {
    virtual ~ExceptionDummyPushDecoratorBase() = default;
    virtual void trigger() = 0;
    bool _isActive{false};
    bool _hasException{false};
  };

  /********************************************************************************************************************/

  template<typename UserType>
  struct ExceptionDummyPushDecorator : NDRegisterAccessorDecorator<UserType>, ExceptionDummyPushDecoratorBase {
    ExceptionDummyPushDecorator(const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>>& target,
        boost::shared_ptr<ExceptionDummy> backend)
    : NDRegisterAccessorDecorator<UserType>(target), _backend(std::move(backend)) {
      assert(_target->isReadable());

      this->_accessModeFlags = _target->getAccessModeFlags();
      this->_accessModeFlags.add(AccessMode::wait_for_new_data);

      _readQueue =
          _myReadQueue.template then<void>([&](Buffer current) { this->_current = current; }, std::launch::deferred);

      _path = _target->getName();
      _path.setAltSeparator(".");
      _path /= "PUSH_READ";
    }

    ~ExceptionDummyPushDecorator() override {
      std::unique_lock<std::mutex> lk(_backend->_pushDecoratorsMutex);
      try {
        auto& list = _backend->_pushDecorators.at(_path);
        for(auto it = list.begin(); it != list.end(); ++it) {
          if(it->lock().get() == nullptr) { // weak_ptr is already not lockable any more
            _backend->_pushDecorators.at(_path).erase(it);
            return;
          }
        }
      }
      catch([[maybe_unused]] std::out_of_range& e) {
      }
      std::cout << "~ExceptionDummyPushDecorator(): Could not unlist instance!" << std::endl;
      assert(false);
    }

    void interrupt() override { this->interrupt_impl(this->_myReadQueue); }

    void setExceptionBackend(boost::shared_ptr<DeviceBackend> exceptionBackend) override {
      // do not set it for the target, since we read from the target in trigger(), but that is the wrong place to
      // call setException(). So we don't call it on our base class NDRegisterAccessorDecorator but on the
      // TransferElement itself.
      // Turn off the linter warning. This is intentional.
      // NOLINTNEXTLINE(bugprone-parent-virtual-call)
      TransferElement::setExceptionBackend(exceptionBackend);
    }

    struct Buffer {
      std::vector<std::vector<UserType>> _data;
      VersionNumber _version;
      DataValidity _validity = {DataValidity::faulty};
    };
    Buffer _current;

    cppext::future_queue<Buffer> _myReadQueue{3};

    void trigger() override {
      try {
        _hasException = false;
        _target->read();
        Buffer b;
        b._data = _target->accessChannels();
        b._version = _backend->_pushVersions[_path];
        b._validity = _target->dataValidity();
        _myReadQueue.push_overwrite(b);
      }
      // fixme: clang tidy says something else is escaping, don't know what
      catch(ChimeraTK::runtime_error&) {
        _isActive = false;
        if(!_hasException) _myReadQueue.push_overwrite_exception(std::current_exception());
        _hasException = true;
      }
    }

    void doPreRead(TransferType) override {
      // do not delegate read transfers to the target
      if(!_backend->isOpen()) {
        throw ChimeraTK::logic_error("Cannot read from closed device.");
      }
    }

    void doPostRead(TransferType, bool updateDataBuffer) override {
      // do not delegate read transfers to the target
      if(updateDataBuffer) {
        // do not update meta data if updateDataBuffer == false, since this is the equivalent to a backend
        // implementation, not a decorator
        _versionNumber = _current._version;
        _dataValidity = _current._validity;
        buffer_2D = _current._data;
      }
    }

    boost::shared_ptr<ExceptionDummy> _backend;
    RegisterPath _path;
    using NDRegisterAccessorDecorator<UserType>::_target;
    using TransferElement::_readQueue;
    using TransferElement::_versionNumber;
    using TransferElement::_dataValidity;
    using NDRegisterAccessor<UserType>::buffer_2D;
  };

  /********************************************************************************************************************/

  // non-template base class for UserType-agnostic insertion into std::map
  struct ExceptionDummyPollDecoratorBase {
    virtual ~ExceptionDummyPollDecoratorBase() = default;
  };

  /// A decorator that returns invalid data for polled variables
  template<typename UserType>
  struct ExceptionDummyPollDecorator : NDRegisterAccessorDecorator<UserType>, ExceptionDummyPollDecoratorBase {
    ExceptionDummyPollDecorator(const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>>& target,
        boost::shared_ptr<ExceptionDummy> backend)
    : NDRegisterAccessorDecorator<UserType>(target), _backend(std::move(backend)) {
      assert(_target->isReadable());

      _path = _target->getName();
      _path.setAltSeparator(".");
    }

    void doPostRead(TransferType type, bool updateDataBuffer) override {
      NDRegisterAccessorDecorator<UserType, UserType>::doPostRead(type, updateDataBuffer);
      // overwriting is only allowed for faulty.
      if(_backend->getValidity(_path) == DataValidity::faulty) this->_dataValidity = DataValidity::faulty;
    }

    boost::shared_ptr<ExceptionDummy> _backend;
    RegisterPath _path;
    using NDRegisterAccessorDecorator<UserType>::_target;
    using TransferElement::_readQueue;
    using TransferElement::_versionNumber;
    using TransferElement::_dataValidity;
  };

  /********************************************************************************************************************/
} // namespace ChimeraTK
