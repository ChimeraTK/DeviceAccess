#include <boost/make_shared.hpp>

#include <exprtk.hpp>

#include "LNMBackendRegisterInfo.h"
#include "LNMAccessorPlugin.h"
#include "NDRegisterAccessorDecorator.h"
#include "TransferElement.h"

namespace ChimeraTK { namespace LNMBackend {

  /********************************************************************************************************************/

  MathPlugin::MathPlugin(
      boost::shared_ptr<LNMBackendRegisterInfo> info, const std::map<std::string, std::string>& parameters)
  : AccessorPlugin(info), _parameters(parameters) {
    // extract parameters
    if(parameters.find("formula") == parameters.end()) {
      throw ChimeraTK::logic_error("LogicalNameMappingBackend MultiplierPlugin: Missing parameter 'formula'.");
    }
  }

  /********************************************************************************************************************/

  void MathPlugin::updateRegisterInfo() {
    auto info = _info.lock();
    auto d = info->_dataDescriptor;
    
    // Change data type to non-integral
    info->_dataDescriptor =
        RegisterInfo::DataDescriptor(d.fundamentalType(), false, false, std::numeric_limits<double>::max_digits10,
            -std::numeric_limits<double>::min_exponent10, DataType::none, d.transportLayerDataType());

    // Fix to unidirectional operation
    if(info->writeable && info->readable) {
      info->readable = false;
    }
  }

  /********************************************************************************************************************/

  struct MathPluginFormulaHelper {
    std::string varName;
    exprtk::expression<double> expression;
    exprtk::symbol_table<double> symbols;
    std::unique_ptr<exprtk::vector_view<double>> valueView;
    std::map<boost::shared_ptr<NDRegisterAccessor<double>>, std::unique_ptr<exprtk::vector_view<double>>> params;

    void compileFormula(const boost::shared_ptr<LogicalNameMappingBackend>& backend,
        const std::map<std::string, std::string>& parameters,
        const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<double>>& target) {
      // create exprtk parser
      exprtk::parser<double> parser;

      // add basic constants like pi
      symbols.add_constants();

      // Create vector view for the value and add it to the symbol table. We need to use a vector view instead of adding
      // the buffer directly as a vector, since our buffers might be swapped and hence the address of the data can
      // change.
      valueView = std::make_unique<exprtk::vector_view<double>>(
          exprtk::make_vector_view(target->accessChannel(0), target->accessChannel(0).size()));
      symbols.add_vector("x", *valueView);

      // iterate parameters, add all but 'formula' parameter as a variable.
      for(auto& parpair : parameters) {
        if(parpair.first == "formula") continue;
        auto acc = backend->getRegisterAccessor<double>(parpair.second, 0, 0, {});
        acc->setExceptionBackend(backend);
        if(acc->getNumberOfChannels() != 1) {
          throw ChimeraTK::logic_error(
              "The LogicalNameMapper MathPlugin supports only scalar or 1D array registers. Register name: '" +
              varName + "', parameter name: '" + parpair.first + "'");
        }
        auto view = std::make_unique<exprtk::vector_view<double>>(
            exprtk::make_vector_view(acc->accessChannel(0), acc->getNumberOfSamples()));
        symbols.add_vector(parpair.first, *view);
        params[acc] = std::move(view);
      }

      // compile the expression
      expression.register_symbol_table(symbols);
      bool success = parser.compile(parameters.at("formula"), expression);
      if(!success) {
        throw ChimeraTK::logic_error("LogicalNameMapping MathPlugin for register '" + varName +
            "': failed to compile expression '" + parameters.at("formula") + "': " + parser.error());
      }
    }

    template<typename T>
    void computeResult(std::vector<double>& x, std::vector<T>& resultBuffer) {
      // inform the value view of the new data pointer - the buffer might have been swapped
      valueView->rebase(x.data());

      // update parameters
      for(auto& p : params) {
        p.first->read();
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
  };

  /********************************************************************************************************************/

  template<typename UserType>
  struct MathPluginDecorator : ChimeraTK::NDRegisterAccessorDecorator<UserType, double> {
    using ChimeraTK::NDRegisterAccessorDecorator<UserType, double>::buffer_2D;

    MathPluginDecorator(boost::shared_ptr<LogicalNameMappingBackend>& backend,
        const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<double>>& target,
        const std::map<std::string, std::string>& parameters, bool isWrite);

    void doPreRead(TransferType type) override {
      if(_isWrite) {
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
    bool _isWrite;

    using ChimeraTK::NDRegisterAccessorDecorator<UserType, double>::_target;

    void setExceptionBackend(boost::shared_ptr<DeviceBackend>) override;
  };

  /********************************************************************************************************************/

  template<typename UserType>
  MathPluginDecorator<UserType>::MathPluginDecorator(boost::shared_ptr<LogicalNameMappingBackend>& backend,
      const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<double>>& target,
      const std::map<std::string, std::string>& parameters, bool isWrite)
  : ChimeraTK::NDRegisterAccessorDecorator<UserType, double>(target), _isWrite(isWrite) {
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

    // compile formula
    h.varName = this->getName();
    h.compileFormula(backend, parameters, target);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void MathPluginDecorator<UserType>::doPostRead(TransferType type, bool hasNewData) {
    _target->postRead(type, hasNewData);
    if(!hasNewData) return;

    // evaluate the expression and store into application buffer
    h.computeResult(_target->accessChannel(0), buffer_2D[0]);

    // update version number and validity from target
    this->_versionNumber = _target->getVersionNumber();
    this->_dataValidity = _target->dataValidity();
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void MathPluginDecorator<UserType>::doPreWrite(TransferType type, ChimeraTK::VersionNumber versionNumber) {
    if(!_isWrite) {
      throw ChimeraTK::logic_error("This register with MathPlugin enabled is not writeable: " + _target->getName());
    }

    // convert from UserType to double - use the target accessor's buffer as a temporary buffer (this is a bit a hack,
    // but it is safe to overwrite the buffer and we can avoid the need for an additional permanent buffer which might
    // not even be used if the register is never written).
    for(size_t k = 0; k < buffer_2D[0].size(); ++k) {
      _target->accessData(0, k) = userTypeToNumeric<double>(buffer_2D[0][k]);
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
        const std::map<std::string, std::string>&, bool) {
      assert(false); // only specialisation is valid
      return {};
    }
  };

  /********************************************************************************************************************/

  template<typename UserType>
  struct MathPlugin_Helper<UserType, double> {
    static boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend, boost::shared_ptr<NDRegisterAccessor<double>>& target,
        const std::map<std::string, std::string>& parameters, bool isWrite) {
      return boost::make_shared<MathPluginDecorator<UserType>>(backend, target, parameters, isWrite);
    }
  };

  /********************************************************************************************************************/

  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> MathPlugin::decorateAccessor(
      boost::shared_ptr<LogicalNameMappingBackend>& backend,
      boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) const {
    return MathPlugin_Helper<UserType, TargetType>::decorateAccessor(
        backend, target, _parameters, _info.lock()->writeable);
  }

  /********************************************************************************************************************/

}} // namespace ChimeraTK::LNMBackend
