#pragma once

#include "BackendFactory.h"
#include "DeviceAccessVersion.h"
#include "DummyBackend.h"
#include "NDRegisterAccessorDecorator.h"

namespace ChimeraTK {

  template<typename UserType>
  struct ExceptionDummyPushDecorator;

  struct ExceptionDummyPushDecoratorBase;

  /********************************************************************************************************************/

  class ExceptionDummy : public ChimeraTK::DummyBackend {
   public:
    ExceptionDummy(std::string mapFileName) : DummyBackend(mapFileName) {
      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
    }

    std::atomic<bool> throwExceptionOpen{false};
    std::atomic<bool> throwExceptionRead{false};
    std::atomic<bool> throwExceptionWrite{false};
    std::atomic<bool> thereHaveBeenExceptions{false};

    static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string> parameters) {
      return boost::shared_ptr<DeviceBackend>(new ExceptionDummy(parameters["map"]));
    }

    void open() override {
      if(throwExceptionOpen) {
        thereHaveBeenExceptions = true;
        throw(ChimeraTK::runtime_error("DummyException: This is a test"));
      }
      ChimeraTK::DummyBackend::open();
      if(throwExceptionRead || throwExceptionWrite) {
        thereHaveBeenExceptions = true;
        throw(ChimeraTK::runtime_error("DummyException: open throws because of "
                                       "device error when already open."));
      }
      thereHaveBeenExceptions = false;
    }

    void close() override {
      setException();
      DummyBackend::close();
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
    }

    bool isFunctional() const override {
      // thereHaveBeenExceptions is different from _hasActiceException
      // * thereHaveBeenExceptions is set when this class originally raised an exception
      // * _hasActiveException is raised externally via setException. This can happen if a transfer element from another backend,
      //   which is in the same logical name mapping backend than this class, has seen an exception.
      return (_opened && !throwExceptionOpen && !throwExceptionRead && !throwExceptionWrite &&
          !thereHaveBeenExceptions && !_hasActiveException);
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
      return acc;
    }

    /// Function to trigger sending values for push-type variables
    void triggerPush(RegisterPath path, VersionNumber v = {});

    void activateAsyncRead() noexcept override;

    void setException() override;

    /// Map of active ExceptionDummyPushDecorator
    std::mutex _pushDecoratorsMutex;
    std::map<RegisterPath, std::list<boost::weak_ptr<ExceptionDummyPushDecoratorBase>>> _pushDecorators;
    std::map<RegisterPath, VersionNumber> _pushVersions;
    bool _activateNewPushAccessors{
        false}; // flag is toggled by activateAsyncRead (true), setException (false) and close (false). Protected by _pushDecoratorMutex

    class BackendRegisterer {
     public:
      BackendRegisterer();
    };
    static BackendRegisterer backendRegisterer;
  };

  /********************************************************************************************************************/

  ExceptionDummy::BackendRegisterer ExceptionDummy::backendRegisterer;

  /********************************************************************************************************************/

  ExceptionDummy::BackendRegisterer::BackendRegisterer() {
    std::cout << "ExceptionDummy::BackendRegisterer: registering backend type ExceptionDummy" << std::endl;
    ChimeraTK::BackendFactory::getInstance().registerBackendType("ExceptionDummy", &ExceptionDummy::createInstance);
  }

  /********************************************************************************************************************/

  struct ExceptionDummyPushDecoratorBase {
    virtual ~ExceptionDummyPushDecoratorBase() {}
    virtual void trigger() = 0;
    bool _isActive{false};
    bool _hasException{false};
  };

  /********************************************************************************************************************/

  template<typename UserType>
  struct ExceptionDummyPushDecorator : NDRegisterAccessorDecorator<UserType>, ExceptionDummyPushDecoratorBase {
    ExceptionDummyPushDecorator(const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>>& target,
        const boost::shared_ptr<ExceptionDummy>& backend)
    : NDRegisterAccessorDecorator<UserType>(target), _backend(backend) {
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
      auto& list = _backend->_pushDecorators.at(_path);
      for(auto it = list.begin(); it != list.end(); ++it) {
        if(it->lock().get() == nullptr) { // weak_ptr is already not lockable any more
          _backend->_pushDecorators.at(_path).erase(it);
          return;
        }
      }
      std::cout << "~ExceptionDummyPushDecorator(): Could not unlist instance!" << std::endl;
      assert(false);
    }

    void interrupt() override { this->interrupt_impl(this->_myReadQueue); }

    void setExceptionBackend(boost::shared_ptr<DeviceBackend> exceptionBackend) override {
      // do not set it for the target, since we read from the target in trigger(), but that is the wrong place to
      // call setException().
      TransferElement::setExceptionBackend(exceptionBackend);
    }

    struct Buffer {
      std::vector<std::vector<UserType>> _data;
      VersionNumber _version;
      DataValidity _validity;
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

  /// Function to trigger sending values for push-type variables
  void ExceptionDummy::triggerPush(RegisterPath path, VersionNumber v) {
    path.setAltSeparator(".");
    std::unique_lock<std::mutex> lk(_pushDecoratorsMutex);
    _pushVersions[path] = v;
    for(auto& acc_weak : _pushDecorators[path]) {
      auto acc = acc_weak.lock();
      if(!acc->_isActive) continue;
      lk.unlock(); // next line might call setException()...
      acc->trigger();
      lk.lock();
    }
  }

  /********************************************************************************************************************/

  void ExceptionDummy::activateAsyncRead() noexcept {
    std::unique_lock<std::mutex> lk(_pushDecoratorsMutex);
    for(auto& pair : _pushDecorators) {
      _pushVersions[pair.first] = {};
      for(auto& acc_weak : pair.second) {
        auto acc = acc_weak.lock();
        if(acc->_isActive) continue;
        lk.unlock();    // next line might call setException()...
        acc->trigger(); // initial value
        lk.lock();
        acc->_isActive = true;
        acc->_hasException = false;
      }
    }
    _activateNewPushAccessors = true;
  }

  /********************************************************************************************************************/

  void ExceptionDummy::setException() {
    DummyBackend::setException();

    // deactivate async transfers
    std::unique_lock<std::mutex> lk(_pushDecoratorsMutex);
    for(auto& pair : _pushDecorators) {
      _pushVersions[pair.first] = {};
      for(auto& acc_weak : pair.second) {
        auto acc = acc_weak.lock();
        if(!acc->_isActive) continue;
        acc->_isActive = false;
        if(acc->_hasException) continue;
        acc->_hasException = true;
        lk.unlock(); // next line might call setException()...
        acc->trigger();
        lk.lock();
      }
    }
    _activateNewPushAccessors = false;
  }

} // namespace ChimeraTK
