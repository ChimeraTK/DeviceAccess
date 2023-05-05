// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LNMMathPlugin.h"

#include "BackendFactory.h"
#include "LNMBackendRegisterInfo.h"
#include "NDRegisterAccessorDecorator.h"
#include "TransferElement.h"

#include <ChimeraTK/cppext/finally.hpp>

namespace ChimeraTK::LNMBackend {

  /********************************************************************************************************************/

  MathPlugin::MathPlugin(const LNMBackendRegisterInfo& info, std::map<std::string, std::string> parameters)
  : AccessorPlugin(info), _parameters(std::move(parameters)) {
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
  }

  /********************************************************************************************************************/

  void MathPlugin::doRegisterInfoUpdate() {
    // Change data type to non-integral
    _info._dataDescriptor = ChimeraTK::DataDescriptor(DataType("float64"));
    _info.supportedFlags.remove(AccessMode::raw);

    // Fix to unidirectional operation
    if(_info.writeable && _info.readable) {
      _info.readable = false;
    }

    _isWrite = _info.writeable;
  }

  /********************************************************************************************************************/

  void MathPlugin::openHook(const boost::shared_ptr<LogicalNameMappingBackend>& backend) {
    // make sure backend catalogue is updated with target backend information
    auto catalogue = backend->getRegisterCatalogue();

    // produce logic_error if MathPlugin has insufficient access rights to parameters
    if(_isWrite && !_info.isWriteable()) {
      throw ChimeraTK::logic_error("MathPlugin target not writeable:" + _info.name);
    }
    for(const auto& parpair : _parameters) {
      auto r = catalogue.getRegister(parpair.second);

      if(!r.isReadable()) {
        throw ChimeraTK::logic_error("MathPlugin parameter not readable:" + parpair.second);
      }
    }

    // If write direction, check for push-type parameters if enabled
    if(_isWrite && _enablePushParameters) {
      for(const auto& parpair : _parameters) {
        auto paramFlags = backend->getRegisterCatalogue().getRegister(parpair.second).getSupportedAccessModes();
        if(paramFlags.has(AccessMode::wait_for_new_data)) {
          _hasPushParameter = true;
          break;
        }
      }
    }
    if(_isWrite && _enablePushParameters && !_hasPushParameter) {
      throw ChimeraTK::logic_error(
          "MathPlugin (writing) with push_parameters requested but no present parameters supports it");
    }
    if(!_isWrite && _enablePushParameters) {
      throw ChimeraTK::logic_error("MathPlugin (reading) does not support push_parameters");
    }

    // we require that all values used in the formula need to be written after open, before we provide first result
    {
      std::unique_lock<std::recursive_mutex> lk(_writeMutex);
      _mainValueWrittenAfterOpen = false;
      _allParametersWrittenAfterOpen = false;
    }

    if(_hasPushParameter) {
      for(const auto& parpair : _parameters) {
        auto pname = parpair.second;
        if(backend->_variables.find(pname) == backend->_variables.end()) {
          // throwing an error here would be ok if we require all parameters of the
          // formula to be LNM variables; i.e. a redirectedRegister as parameter would not work then.
          // However we do have servers using that currently.
          // throw logic_error("no LNM variable defined for parameter " + pname);
        }
        else {
          // add this only if not yet in there, otherwise re-open of device would blow up size.
          backend->_variables[pname].usingFormulas.push_back(this);
        }
      }
    }
  }

  /********************************************************************************************************************/

  boost::shared_ptr<MathPluginFormulaHelper> MathPlugin::getFormulaHelper(
      boost::shared_ptr<LogicalNameMappingBackend> backend) {
    // lock against concurrent creation
    std::unique_lock<std::recursive_mutex> lk{_writeMutex};
    auto p = _h.lock();
    if(!p) {
      if(!backend) {
        return nullptr;
      }
      _creatingFormulaHelper = true;
      p = boost::make_shared<MathPluginFormulaHelper>(this, backend);
      _creatingFormulaHelper = false;
      _h = p;
    }
    return p;
  }

  /********************************************************************************************************************/

  void MathPluginFormulaHelper::updateResult(ChimeraTK::VersionNumber versionNumber) {
    assert(_lastMainValue.size() == _target->getNumberOfSamples());

    try {
      std::unique_lock<std::recursive_mutex> lk(_mp->_writeMutex);

      auto paramDataValidity = ChimeraTK::DataValidity::ok;

      //  update parameters
      for(auto& p : params) {
        p.first->readLatest();
        // check the DataValidity of parameter.
        if(p.first->dataValidity() == ChimeraTK::DataValidity::faulty) {
          // probably compiler optimize it automatically and assign it only once.
          paramDataValidity = ChimeraTK::DataValidity::faulty;
        }
      }

      if(!checkAllParametersWritten() || !_mp->_mainValueWrittenAfterOpen) {
        return;
      }

      computeResult(_lastMainValue, _target->accessChannel(0));
      // pass validity to target and delegate preWrite
      if(paramDataValidity == ChimeraTK::DataValidity::ok && _lastMainValidity == ChimeraTK::DataValidity::ok) {
        _target->setDataValidity(ChimeraTK::DataValidity::ok);
      }
      else {
        _target->setDataValidity(ChimeraTK::DataValidity::faulty);
      }

      _target->writeDestructively(versionNumber);
    }
    catch(runtime_error&) {
      // runtime_error from param.readLatest() or target->write()
      // we could actually even ignore it, since we don't expect exceptions on param.readLatest(), and target->write()
      // already puts backend into exception state.
      // TODO discuss - should we direct to lnmBackend->getExceptionBackend() instead?
      _target->getExceptionBackend()->setException();
    }
  }

  /********************************************************************************************************************/

  void MathPlugin::closeHook() {}

  /********************************************************************************************************************/

  void MathPlugin::exceptionHook() {
    closeHook();
  }

  /********************************************************************************************************************/

  bool MathPluginFormulaHelper::checkAllParametersWritten() {
    if(_mp->_allParametersWrittenAfterOpen) {
      return true;
    }

    for(const auto& acc : _accessorMap) {
      // push-type accessors: version number check will notice whether valid data has been provided after open
      // poll-type accessors: version number check always succeeds since readLatest returns new version numbers
      if(acc.second->getVersionNumber() <= _backend->getVersionOnOpen()) {
        return false;
      }
    }
    _mp->_allParametersWrittenAfterOpen = true;
    return _mp->_allParametersWrittenAfterOpen;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  struct MathPluginDecorator : ChimeraTK::NDRegisterAccessorDecorator<UserType, double> {
    using ChimeraTK::NDRegisterAccessorDecorator<UserType, double>::buffer_2D;

    MathPluginDecorator(boost::shared_ptr<LogicalNameMappingBackend>& backend,
        const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<double>>& target, MathPlugin* p);

    void doPreRead(TransferType type) override {
      if(_p->_isWrite) {
        throw ChimeraTK::logic_error("This register with MathPlugin enabled is not readable: " + _target->getName());
      }
      _target->preRead(type);
    }

    // registers are either readable or writeable
    [[nodiscard]] bool isReadable() const override { return !_p->_isWrite; }

    void doPostRead(TransferType type, bool hasNewData) override;

    void doPreWrite(TransferType type, VersionNumber versionNumber) override;

    void doPostWrite(TransferType type, VersionNumber versionNumber) override;

    bool doWriteTransfer(ChimeraTK::VersionNumber) override;
    bool doWriteTransferDestructively(ChimeraTK::VersionNumber) override;

    boost::shared_ptr<MathPluginFormulaHelper> _h;
    MathPlugin* _p;

    // If not all parameters have been updated in the plugin, all parts of the write
    // transaction (preWrite, writeTransfer and postWrite) are not delegated to the target
    bool _skipWriteDelegation{false};

    using ChimeraTK::NDRegisterAccessorDecorator<UserType, double>::_target;

    void setExceptionBackend(boost::shared_ptr<DeviceBackend>) override;
  };

  /********************************************************************************************************************/

  template<typename UserType>
  MathPluginDecorator<UserType>::MathPluginDecorator(boost::shared_ptr<LogicalNameMappingBackend>& backend,
      const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<double>>& target, MathPlugin* p)
  : ChimeraTK::NDRegisterAccessorDecorator<UserType, double>(target), _p(p) {
    // 2D arrays are not yet supported
    if(_target->getNumberOfChannels() != 1) {
      throw ChimeraTK::logic_error(
          "The LogicalNameMapper MathPlugin supports only scalar or 1D array registers. Register name: " +
          this->getName());
    }

    // Although this is a decorator we do not use the exceptions backend from the target because this decorator is
    // given out by a backend
    target->setExceptionBackend(backend);
    this->_exceptionBackend = backend;

    _h = _p->getFormulaHelper(backend);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void MathPluginDecorator<UserType>::doPostRead(TransferType type, bool hasNewData) {
    _target->postRead(type, hasNewData);
    if(!hasNewData) return;

    // update parameters
    auto paramDataValidity = ChimeraTK::DataValidity::ok;
    for(auto& p : _h->params) {
      p.first->readLatest();
      if(p.first->dataValidity() == ChimeraTK::DataValidity::faulty) {
        // probably compiler optimize it automatically and assign it only once.
        paramDataValidity = ChimeraTK::DataValidity::faulty;
      }
    }

    // evaluate the expression and store into application buffer
    _h->computeResult(_target->accessChannel(0), buffer_2D[0]);

    // update version number and validity from target
    this->_versionNumber = _target->getVersionNumber();
    this->_dataValidity = _target->dataValidity();
    if(paramDataValidity == ChimeraTK::DataValidity::faulty) {
      this->_dataValidity = ChimeraTK::DataValidity::faulty;
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void MathPluginDecorator<UserType>::doPreWrite(TransferType type, ChimeraTK::VersionNumber versionNumber) {
    if(!_p->_isWrite) {
      throw ChimeraTK::logic_error("This register with MathPlugin enabled is not writeable: " + _target->getName());
    }
    // LNM backend
    auto backend = _h->_backend;
    if(!backend->isOpen()) {
      throw ChimeraTK::logic_error("LNM backend not opened!");
    }
    if(!backend->isFunctional()) {
      std::cout << "previous unrecovered error while trying to write: " + this->getName() << std::endl;
      throw ChimeraTK::runtime_error("previous unrecovered error while trying to write: " + this->getName());
    }

    // The readLatest might throw an exception. In this case preWrite() is never delegated and we must not call the
    // target's postWrite().
    _skipWriteDelegation = true;

    auto paramDataValidity = ChimeraTK::DataValidity::ok;
    // update parameters
    for(auto& p : _h->params) {
      p.first->readLatest();
      // check the DataValidity of parameter.
      if(p.first->dataValidity() == ChimeraTK::DataValidity::faulty) {
        // probably compiler optimize it automatically and assign it only once.
        paramDataValidity = ChimeraTK::DataValidity::faulty;
      }
    }

    // convert from UserType to double - use the target accessor's buffer as a temporary buffer (this is a bit a hack,
    // but it is safe to overwrite the buffer and we can avoid the need for an additional permanent buffer which might
    // not even be used if the register is never written).
    for(size_t k = 0; k < buffer_2D[0].size(); ++k) {
      _target->accessData(0, k) = userTypeToNumeric<double>(buffer_2D[0][k]);
    }

    // update last written data buffer for other threads if needed
    if(_p->_hasPushParameter) {
      // Accquire the lock and hold it until the transaction is completed in postWrite.
      // This is safe because it is guaranteed by the framework that pre- and post actions are called in pairs.
      _p->_writeMutex.lock();
      _h->_lastMainValue = _target->accessChannel(0);
      _h->_lastMainValidity = _target->dataValidity();
      _p->_mainValueWrittenAfterOpen = true;

      if(!_h->checkAllParametersWritten()) {
        return;
      }
    }

    // There either are only poll-type parameters, or all push-type parameters have been received.
    // We are OK to go through with the transfer
    _skipWriteDelegation = false;

    // evaluate the expression and store into target accessor
    _h->computeResult(_target->accessChannel(0), _target->accessChannel(0));

    // pass validity to target and delegate preWrite
    if(paramDataValidity == ChimeraTK::DataValidity::ok && this->_dataValidity == ChimeraTK::DataValidity::ok) {
      _target->setDataValidity(ChimeraTK::DataValidity::ok);
    }
    else {
      _target->setDataValidity(ChimeraTK::DataValidity::faulty);
    }
    _target->preWrite(type, versionNumber);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  bool MathPluginDecorator<UserType>::doWriteTransfer(ChimeraTK::VersionNumber versionNumber) {
    if(_skipWriteDelegation) {
      return false; // No data loss. Value has been stored in preWrite for the parameters thread.
    }

    return _target->writeTransfer(versionNumber);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  bool MathPluginDecorator<UserType>::doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber) {
    return doWriteTransfer(versionNumber);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void MathPluginDecorator<UserType>::doPostWrite(TransferType type, ChimeraTK::VersionNumber versionNumber) {
    if(_skipWriteDelegation && (this->_activeException != nullptr)) {
      // Something has thrown before the target's preWrite was called. Re-throw it here.
      // Do not unlock the mutex. It never has been locked.
      std::rethrow_exception(this->_activeException);
    }

    // make sure the mutex is released, even if the delegated postWrite kicks out with an exception
    auto _ = cppext::finally([&] {
      if(_p->_hasPushParameter) {
        _p->_writeMutex.unlock();
      }
    });

    if(_skipWriteDelegation) {
      return; // the trarget preWrite() has not been executed, so stop here
    }

    // delegate to the target
    _target->setActiveException(this->_activeException);
    _target->postWrite(type, versionNumber);
    // (the "finally" lambda releasing the lock is executed here latest)
  } // namespace LNMBackend

  /********************************************************************************************************************/

  template<typename UserType>
  void MathPluginDecorator<UserType>::setExceptionBackend(boost::shared_ptr<DeviceBackend> exceptionBackend) {
    this->_exceptionBackend = exceptionBackend;
    _target->setExceptionBackend(exceptionBackend);
    // params is a map with NDRegisterAccessors as key
    for(auto& p : _h->params) {
      p.first->setExceptionBackend(exceptionBackend);
    }
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> MathPlugin::decorateAccessor(
      boost::shared_ptr<LogicalNameMappingBackend>& backend, boost::shared_ptr<NDRegisterAccessor<TargetType>>& target,
      const UndecoratedParams&) {
    if constexpr(std::is_same_v<TargetType, double>) {
      return boost::make_shared<MathPluginDecorator<UserType>>(backend, target, this);
    }
    else {
      // this code branch is compiled because of mpl for loop over types, but must never run
      assert(false);
    }
  }

  /********************************************************************************************************************/

  MathPluginFormulaHelper::MathPluginFormulaHelper(
      MathPlugin* p, const boost::shared_ptr<LogicalNameMappingBackend>& backend)
  : _backend(backend), _mp(p) {
    auto* info = _mp->info();
    auto length = info->length;

    if(_mp->_hasPushParameter) {
      for(const auto& parpair : _mp->_parameters) {
        // push-type parameters need to be obtained with wait_for_new_data, others without
        AccessModeFlags flags{};
        auto paramFlags = backend->getRegisterCatalogue().getRegister(parpair.second).getSupportedAccessModes();
        if(paramFlags.has(AccessMode::wait_for_new_data)) {
          flags = {AccessMode::wait_for_new_data};
          // We only allow push-type parameters for Variables defined via LogicalNameMapping.
          // If we allowed other cases (e.g. redirected registers to a device supporting wait_for_new_data, it
          // would be hard to define when pushParameterThread should be stopped
          if(backend->_catalogue_mutable.getBackendRegister(parpair.second).targetType !=
              LNMBackendRegisterInfo::VARIABLE) {
            throw logic_error("only LNM defined variables allowed as push parameters!");
          }
        }
        auto acc = backend->getRegisterAccessor<double>(parpair.second, 0, 0, flags);
        if(acc->getNumberOfChannels() != 1) {
          throw ChimeraTK::logic_error(
              "The LogicalNameMapper MathPlugin supports only scalar or 1D array registers. Register name: '" +
              info->name + "', parameter name: '" + parpair.first + "'");
        }
        _accessorMap[parpair.first] = acc;
      }
      _lastMainValue.resize(length);
    }
    else {
      // obtain poll-type accessors for parameters
      for(const auto& par : _mp->_parameters) {
        _accessorMap[par.first] = backend->getRegisterAccessor<double>(par.second, 0, 0, {});
      }
    }

    // compile formula
    compileFormula(_mp->_formula, _accessorMap, length);
    varName = info->name;

    boost::shared_ptr<DeviceBackend> targetDevice;
    if(info->deviceName == "this") {
      targetDevice = _backend;
    }
    else {
      targetDevice = BackendFactory::getInstance().createBackend(info->deviceName);
    }
    _target = targetDevice->getRegisterAccessor<double>(info->registerName, info->length, info->firstIndex, {});
  }

  /********************************************************************************************************************/

  void MathPluginFormulaHelper::compileFormula(const std::string& formula,

      const std::map<std::string, boost::shared_ptr<ChimeraTK::NDRegisterAccessor<double>>>& parameters,
      size_t nElements) {

    // create exprtk parser
    exprtk::parser<double> parser;

    // add basic constants like pi
    symbols.add_constants();

    // Add vector manipulation functions
    symbols.add_package(vecOpsPkg);

    // Create vector view for the value and add it to the symbol table. We need to use a vector view instead of adding
    // the buffer directly as a vector, since our buffers might be swapped and hence the address of the data can
    // change. The pointer used for the view is for now to a temporary vector and will become invalid once this
    // function returns. This is acceptable, since before using the view the pointer will be updated to the right
    // buffer. Using a nullptr instead of the temporary buffer does not work, as the buffer seems to be accessible
    // during compilation of the formula.
    std::vector<double> temp(nElements);
    valueView = std::make_unique<exprtk::vector_view<double>>(exprtk::make_vector_view(temp, nElements));
    symbols.add_vector("x", *valueView);

    // iterate parameters, add all but 'formula' parameter as a variable.
    for(const auto& parpair : parameters) {
      if(parpair.first == "formula") continue;
      const auto& acc = parpair.second;
      acc->setExceptionBackend(_backend);
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

} // namespace ChimeraTK::LNMBackend
