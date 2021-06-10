#include <boost/make_shared.hpp>

#include <exprtk.hpp>

#include "LNMBackendRegisterInfo.h"
#include "LNMAccessorPlugin.h"
#include "NDRegisterAccessorDecorator.h"
#include "TransferElement.h"
#include "ReadAnyGroup.h"
#include "BackendFactory.h"

namespace ChimeraTK { namespace LNMBackend {

  /********************************************************************************************************************/

  struct MathPluginFormulaHelper {
    std::string varName;
    exprtk::expression<double> expression;
    exprtk::symbol_table<double> symbols;
    std::unique_ptr<exprtk::vector_view<double>> valueView;
    std::map<boost::shared_ptr<NDRegisterAccessor<double>>, std::unique_ptr<exprtk::vector_view<double>>> params;

    void compileFormula(const std::string& formula, const boost::shared_ptr<LogicalNameMappingBackend>& backend,
        const std::map<std::string, boost::shared_ptr<ChimeraTK::NDRegisterAccessor<double>>>& parameters,
        size_t nElements);

    template<typename T>
    void computeResult(std::vector<double>& x, std::vector<T>& resultBuffer);
  };

  /********************************************************************************************************************/

  MathPlugin::MathPlugin(
      boost::shared_ptr<LNMBackendRegisterInfo> info, const std::map<std::string, std::string>& parameters)
  : AccessorPlugin(info), _parameters(parameters) {
    // extract parameters
    if(_parameters.find("formula") == _parameters.end()) {
      throw ChimeraTK::logic_error("LogicalNameMappingBackend MultiplierPlugin: Missing parameter 'formula'.");
    }
    _formula = _parameters.at("formula");
    _parameters.erase("formula");
    if(_parameters.find("enable_push_parameters") != _parameters.end()) {
      _enablePushParameters = true;
      _parameters.erase("enable_push_parameters");
    }
    // create MathPluginFormulaHelper and set name
    _h = boost::make_shared<MathPluginFormulaHelper>();
    _h->varName = info->name;
    _info = info;
  }

  /********************************************************************************************************************/

  void MathPlugin::updateRegisterInfo() {
    auto info = _info.lock();

    // Change data type to non-integral
    info->_dataDescriptor = RegisterInfo::DataDescriptor(DataType("float64"));

    // Fix to unidirectional operation
    if(info->writeable && info->readable) {
      info->readable = false;
    }
    _isWrite = _info.lock()->writeable;
  }

  /********************************************************************************************************************/

  void MathPlugin::openHook(boost::shared_ptr<LogicalNameMappingBackend> backend) {
    // make sure backend catalogue is updated with target backend information
    backend->getRegisterCatalogue();

    // check if this is the first call to openHook: perform tasks which could not be done in the constructor because
    // the backend is not yet available there.
    if(_backend._empty()) {
      // make sure the register info is up to date
      updateRegisterInfo();

      // store backend as weak pointer for later use
      _backend = backend;

      // If write direction, check for push-type parameters if enabled
      if(_isWrite && _enablePushParameters) {
        for(auto& parpair : _parameters) {
          auto paramFlags = backend->getRegisterCatalogue().getRegister(parpair.second)->getSupportedAccessModes();
          if(paramFlags.has(AccessMode::wait_for_new_data)) {
            _hasPushParameter = true;
            break;
          }
        }

        // perpare for updates via push-type parameters
        if(_hasPushParameter) {
          // fill the _pushParameterReadGroup
          for(auto& parpair : _parameters) {
            // push-type parameters need to be obtained with wait_for_new_data, others without
            AccessModeFlags flags{};
            auto paramFlags = backend->getRegisterCatalogue().getRegister(parpair.second)->getSupportedAccessModes();
            if(paramFlags.has(AccessMode::wait_for_new_data)) {
              flags = {AccessMode::wait_for_new_data};
            }
            auto acc = backend->getRegisterAccessor<double>(parpair.second, 0, 0, flags);
            if(acc->getNumberOfChannels() != 1) {
              throw ChimeraTK::logic_error(
                  "The LogicalNameMapper MathPlugin supports only scalar or 1D array registers. Register name: '" +
                  _info.lock()->name + "', parameter name: '" + parpair.first + "'");
            }
            _pushParameterReadGroup.add(acc);
            _pushParameterAccessorMap[parpair.first] = acc;
          }
          _pushParameterReadGroup.finalise();
          _lastWrittenValue.resize(_info.lock()->length);
        }

        // compile formula
        try {
          _h->compileFormula(_formula, _backend.lock(), _pushParameterAccessorMap, _info.lock()->length);
        }
        catch(...) {
          // Do not throw errors at this point, only when accessing the register directly
          _hasPushParameter = false;
        }
      }
    }

    // if we have push-type parameters triggering a write of the target, start the _pushParameterWriteThread
    if(_hasPushParameter) {
      assert(_isWrite); // we do not set hasPushParameter if !isWrite

      // If the thread is still running, it will terminate soon by itself. We must not attempt to terminate it here,
      // because it would be subject to the race condition that the thread still sees an exception from before recovery
      // which results in putting the device back into exception state.
      // The reason why we might need to wait here is that we cannot wait for the thread to terminate in the exception
      // hook, because it might be called from inside the thread.
      while(_pushParameterWriteThread.joinable()) {
        _pushParameterReadGroup.interrupt();
        _pushParameterWriteThread.try_join_for(boost::chrono::milliseconds(100));
      }

      // start write thread
      boost::barrier waitUntilThreadLaunched(2);
      _pushParameterWriteThread = boost::thread([this, &waitUntilThreadLaunched] {
        // obtain target accessor
        auto targetDevice = BackendFactory::getInstance().createBackend(_info.lock()->deviceName);
        auto info = _info.lock();
        auto target = targetDevice->getRegisterAccessor<double>(info->registerName, info->length, info->firstIndex, {});
        // empty all queues (initial values, remaining exceptions from previous thread runs). Ignore all exceptions.
        while(true) {
          auto notfy = _pushParameterReadGroup.waitAnyNonBlocking();
          if(!notfy.isReady()) break;
          try {
            notfy.accept();
          }
          catch(...) {
            continue;
          }
        }
        assert(!_backend.lock()->_asyncReadActive);
        assert(_lastWrittenValue.size() == target->getNumberOfSamples());

        // need to get to this point before the openHook completes
        waitUntilThreadLaunched.wait();

        // start processing loop
        while(true) {
          try {
            {
              std::unique_lock<std::mutex> lk(_mx_lastWrittenValue);
              _h->computeResult(_lastWrittenValue, target->accessChannel(0));
            }
            target->writeDestructively();
            _pushParameterReadGroup.readAny();
          }
          catch(ChimeraTK::runtime_error& e) {
            // no need to report the exception, as the accessor should have already done it
            return;
          }
        }
      });

      // do not proceed before the thread is ready to receive new data
      waitUntilThreadLaunched.wait();
    }
  }

  /********************************************************************************************************************/

  void MathPlugin::closeHook() {
    // terminate _pushParameterWriteThread if running
    if(_hasPushParameter) {
      // if called from within _pushParameterWriteThread, we must not joing, but there nothing to do anyway, as will
      // the thread will terminate due to the exception!
      if(boost::this_thread::get_id() != _pushParameterWriteThread.get_id()) {
        while(_pushParameterWriteThread.joinable()) {
          _pushParameterReadGroup.interrupt();
          _pushParameterWriteThread.try_join_for(boost::chrono::milliseconds(100));
        }
      }
    }
  }

  /********************************************************************************************************************/

  void MathPlugin::exceptionHook() {
    // terminate _pushParameterWriteThread if running, just as device is closed
    closeHook();
  }

  /********************************************************************************************************************/

  template<typename UserType>
  struct MathPluginDecorator : ChimeraTK::NDRegisterAccessorDecorator<UserType, double> {
    using ChimeraTK::NDRegisterAccessorDecorator<UserType, double>::buffer_2D;

    MathPluginDecorator(boost::shared_ptr<LogicalNameMappingBackend>& backend,
        const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<double>>& target,
        const std::map<std::string, std::string>& parameters, MathPlugin* p);

    void doPreRead(TransferType type) override {
      if(_p->_isWrite) {
        throw ChimeraTK::logic_error("This register with MathPlugin enabled is not readable: " + _target->getName());
      }
      _target->preRead(type);
    }

    void doPostRead(TransferType type, bool hasNewData) override;

    void doPreWrite(TransferType type, VersionNumber versionNumber) override;

    void doPostWrite(TransferType type, VersionNumber versionNumber) override {
      _target->postWrite(type, versionNumber);
    }

    MathPluginFormulaHelper h;
    MathPlugin* _p;

    using ChimeraTK::NDRegisterAccessorDecorator<UserType, double>::_target;

    void setExceptionBackend(boost::shared_ptr<DeviceBackend>) override;
  };

  /********************************************************************************************************************/

  template<typename UserType>
  MathPluginDecorator<UserType>::MathPluginDecorator(boost::shared_ptr<LogicalNameMappingBackend>& backend,
      const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<double>>& target,
      const std::map<std::string, std::string>& parameters, MathPlugin* p)
  : ChimeraTK::NDRegisterAccessorDecorator<UserType, double>(target), _p(p) {
    // 2D arrays are not yet supported
    if(_target->getNumberOfChannels() != 1) {
      throw ChimeraTK::logic_error(
          "The LogicalNameMapper MathPlugin supports only scalar or 1D array registers. Register name: " +
          this->getName());
    }

    // Although this is a decorator we do not use the exceptions backend from the target because this decorator is given
    // out by a backend
    target->setExceptionBackend(backend);
    this->_exceptionBackend = backend;

    // obtain accessors for parameters
    std::map<std::string, boost::shared_ptr<ChimeraTK::NDRegisterAccessor<double>>> accessorMap;
    for(auto& par : parameters) {
      accessorMap[par.first] = backend->getRegisterAccessor<double>(par.second, 0, 0, {});
    }

    // compile formula
    h.varName = this->getName();
    h.compileFormula(_p->_formula, backend, accessorMap, target->getNumberOfSamples());
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void MathPluginDecorator<UserType>::doPostRead(TransferType type, bool hasNewData) {
    _target->postRead(type, hasNewData);
    if(!hasNewData) return;

    // update parameters
    for(auto& p : h.params) p.first->readLatest();

    // evaluate the expression and store into application buffer
    h.computeResult(_target->accessChannel(0), buffer_2D[0]);

    // update version number and validity from target
    this->_versionNumber = _target->getVersionNumber();
    this->_dataValidity = _target->dataValidity();
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void MathPluginDecorator<UserType>::doPreWrite(TransferType type, ChimeraTK::VersionNumber versionNumber) {
    if(!_p->_isWrite) {
      throw ChimeraTK::logic_error("This register with MathPlugin enabled is not writeable: " + _target->getName());
    }

    // update parameters
    for(auto& p : h.params) p.first->readLatest();

    // convert from UserType to double - use the target accessor's buffer as a temporary buffer (this is a bit a hack,
    // but it is safe to overwrite the buffer and we can avoid the need for an additional permanent buffer which might
    // not even be used if the register is never written).
    for(size_t k = 0; k < buffer_2D[0].size(); ++k) {
      _target->accessData(0, k) = userTypeToNumeric<double>(buffer_2D[0][k]);
    }

    // update last written data buffer for updater thread if needed
    if(_p->_hasPushParameter) {
      std::unique_lock<std::mutex> lk(_p->_mx_lastWrittenValue);
      _p->_lastWrittenValue = _target->accessChannel(0);
    }

    // evaluate the expression and store into target accessor
    h.computeResult(_target->accessChannel(0), _target->accessChannel(0));

    // pass validity to target and delegate preWrite
    _target->setDataValidity(this->_dataValidity);
    _target->preWrite(type, versionNumber);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void MathPluginDecorator<UserType>::setExceptionBackend(boost::shared_ptr<DeviceBackend> exceptionBackend) {
    this->_exceptionBackend = exceptionBackend;
    _target->setExceptionBackend(exceptionBackend);
    // params is a map with NDRegisterAccessors as key
    for(auto& p : h.params) {
      p.first->setExceptionBackend(exceptionBackend);
    }
  }

  /********************************************************************************************************************/

  /** Helper class to implement MultiplierPlugin::decorateAccessor (can later be realised with if constexpr) */
  template<typename UserType, typename TargetType>
  struct MathPlugin_Helper {
    static boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>&, boost::shared_ptr<NDRegisterAccessor<TargetType>>&,
        const std::map<std::string, std::string>&, MathPlugin*) {
      assert(false); // only specialisation is valid
      return {};
    }
  };

  /********************************************************************************************************************/

  template<typename UserType>
  struct MathPlugin_Helper<UserType, double> {
    static boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend, boost::shared_ptr<NDRegisterAccessor<double>>& target,
        const std::map<std::string, std::string>& parameters, MathPlugin* p) {
      return boost::make_shared<MathPluginDecorator<UserType>>(backend, target, parameters, p);
    }
  };

  /********************************************************************************************************************/

  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> MathPlugin::decorateAccessor(
      boost::shared_ptr<LogicalNameMappingBackend>& backend,
      boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) {
    return MathPlugin_Helper<UserType, TargetType>::decorateAccessor(backend, target, _parameters, this);
  }

  /********************************************************************************************************************/

  void MathPluginFormulaHelper::compileFormula(const std::string& formula,
      const boost::shared_ptr<LogicalNameMappingBackend>& backend,
      const std::map<std::string, boost::shared_ptr<ChimeraTK::NDRegisterAccessor<double>>>& parameters,
      size_t nElements) {
    // create exprtk parser
    exprtk::parser<double> parser;

    // add basic constants like pi
    symbols.add_constants();

    // Create vector view for the value and add it to the symbol table. We need to use a vector view instead of adding
    // the buffer directly as a vector, since our buffers might be swapped and hence the address of the data can
    // change. The pointer used for the view is for now to a temporary vector and will become invalid once this function
    // returns. This is acceptable, since before using the view the pointer will be updated to the right buffer.
    // Using a nullptr instead of the temporary buffer does not work, as the buffer seems to be accessible during
    // compilation of the formula.
    std::vector<double> temp(nElements);
    valueView = std::make_unique<exprtk::vector_view<double>>(exprtk::make_vector_view(temp, nElements));
    symbols.add_vector("x", *valueView);

    // iterate parameters, add all but 'formula' parameter as a variable.
    for(auto& parpair : parameters) {
      if(parpair.first == "formula") continue;
      auto& acc = parpair.second;
      acc->setExceptionBackend(backend);
      if(acc->getNumberOfChannels() != 1) {
        throw ChimeraTK::logic_error(
            "The LogicalNameMapper MathPlugin supports only scalar or 1D array registers. Register name: '" + varName +
            "', parameter name: '" + parpair.first + "'");
      }
      auto view = std::make_unique<exprtk::vector_view<double>>(
          exprtk::make_vector_view(acc->accessChannel(0), acc->getNumberOfSamples()));
      symbols.add_vector(parpair.first, *view);
      params[acc] = std::move(view);
    }

    // compile the expression
    expression.register_symbol_table(symbols);
    bool success = parser.compile(formula, expression);
    if(!success) {
      throw ChimeraTK::logic_error("LogicalNameMapping MathPlugin for register '" + varName +
          "': failed to compile expression '" + formula + "': " + parser.error());
    }
  }

  /********************************************************************************************************************/

  template<typename T>
  void MathPluginFormulaHelper::computeResult(std::vector<double>& x, std::vector<T>& resultBuffer) {
    // inform the value view of the new data pointer - the buffer might have been swapped
    valueView->rebase(x.data());

    // update parameter buffers
    for(auto& p : params) {
      p.second->rebase(p.first->accessChannel(0).data());
    }

    // evaluate the expression, obtain the result in a way so it also works when using the return statement
    double valueWhenNotUsingReturn = expression.value();
    exprtk::results_context<double> results = expression.results();

    // extract result depending on how it was returned
    if(results.count() == 0) {
      // if results.count() is 0, the return statement presumably has not been used
      if(resultBuffer.size() != 1) {
        throw ChimeraTK::logic_error("LogicalNameMapping MathPlugin for register '" + varName +
            "': The expression returns a scalar but " + std::to_string(resultBuffer.size()) + " expected.");
      }
      resultBuffer[0] = numericToUserType<T>(valueWhenNotUsingReturn);
    }
    else if(results.count() == 1) {
      // return statement has been used to return exactly one value (note: this value might be an array)
      exprtk::type_store<double> result = results[0];

      // make sure we got a numeric result
      if(result.type != exprtk::type_store<double>::e_scalar && result.type != exprtk::type_store<double>::e_vector) {
        throw ChimeraTK::logic_error("LogicalNameMapping MathPlugin for register '" + varName +
            "': The expression did not return a numeric result.");
      }

      // create vector view and check that its size matches
      exprtk::type_store<double>::type_view<double> view(result);
      if(view.size() != resultBuffer.size()) {
        throw ChimeraTK::logic_error("LogicalNameMapping MathPlugin for register '" + varName +
            "': The expression returns " + std::to_string(view.size()) + " elements but " +
            std::to_string(resultBuffer.size()) + " expected.");
      }

      // convert and copy data into target buffer
      for(size_t k = 0; k < view.size(); ++k) {
        resultBuffer[k] = numericToUserType<T>(view[k]);
      }
    }
    else {
      // multiple results in return statement are unexpected
      throw ChimeraTK::logic_error("LogicalNameMapping MathPlugin for register '" + varName +
          "': The expression returned " + std::to_string(results.count()) + " results, expect exactly one result.");
    }
  }

  /********************************************************************************************************************/

}} // namespace ChimeraTK::LNMBackend
