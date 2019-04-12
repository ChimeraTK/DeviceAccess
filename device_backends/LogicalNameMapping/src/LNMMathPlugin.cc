#include <boost/make_shared.hpp>

#include <exprtk.hpp>

#include "LNMBackendRegisterInfo.h"
#include "LNMAccessorPlugin.h"
#include "NDRegisterAccessorDecorator.h"

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
    // Change data type to non-integral
    auto d = _info->_dataDescriptor;
    _info->_dataDescriptor = ChimeraTK::RegisterInfo::DataDescriptor(d.fundamentalType(), false, false,
        std::numeric_limits<double>::max_digits10, -std::numeric_limits<double>::min_exponent10, d.rawDataType(),
        d.transportLayerDataType());
  }

  /********************************************************************************************************************/

  template<typename UserType>
  struct MathPluginDecorator : ChimeraTK::NDRegisterAccessorDecorator<UserType, double> {
    using ChimeraTK::NDRegisterAccessorDecorator<UserType, double>::buffer_2D;

    MathPluginDecorator(boost::shared_ptr<LogicalNameMappingBackend>& backend,
        const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<double>>& target,
        const std::map<std::string, std::string>& parameters);

    void doPreRead() override { _target->preRead(); }

    void doPostRead() override;

    void doPreWrite() override;

    void doPostWrite() override { _target->postWrite(); }

    void interrupt() override { _target->interrupt(); }

    ChimeraTK::VersionNumber getVersionNumber() const override { return _target->getVersionNumber(); }

    exprtk::expression<double> expression;
    exprtk::symbol_table<double> symbols;
    std::unique_ptr<exprtk::vector_view<double>> valueView;
    std::map<boost::shared_ptr<NDRegisterAccessor<double>>, std::unique_ptr<exprtk::vector_view<double>>> params;

    using ChimeraTK::NDRegisterAccessorDecorator<UserType, double>::_target;
  };

  /********************************************************************************************************************/

  template<typename UserType>
  MathPluginDecorator<UserType>::MathPluginDecorator(boost::shared_ptr<LogicalNameMappingBackend>& backend,
      const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<double>>& target,
      const std::map<std::string, std::string>& parameters)
  : ChimeraTK::NDRegisterAccessorDecorator<UserType, double>(target) {
    // 2D arrays are not yet supported
    if(_target->getNumberOfChannels() != 1) {
      throw ChimeraTK::logic_error(
          "The LogicalNameMapper MathPlugin supports only scalar or 1D array registers. Register name: " +
          this->getName());
    }

    // create exprtk parser
    exprtk::parser<double> parser;

    // add basic constants like pi
    symbols.add_constants();

    // Create vector view for the value and add it to the symbol table. We need to use a vector view instead of adding
    // the buffer directly as a vector, since our buffers might be swapped and hence the address of the data can
    // change.
    valueView = std::make_unique<exprtk::vector_view<double>>(
        exprtk::make_vector_view(_target->accessChannel(0), _target->accessChannel(0).size()));
    symbols.add_vector("x", *valueView);

    // iterate parameters, add all but 'formula' parameter as a variable
    for(auto& parpair : parameters) {
      if(parpair.first == "formula") continue;
      auto acc = backend->getRegisterAccessor<double>(parpair.second, 0, 0, {});
      if(acc->getNumberOfChannels() != 1) {
        throw ChimeraTK::logic_error(
            "The LogicalNameMapper MathPlugin supports only scalar or 1D array registers. Register name: '" +
            this->getName() + "', parameter name: '" + parpair.first + "'");
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
      throw ChimeraTK::logic_error("LogicalNameMapping MathPlugin for register '" + this->getName() +
          "': failed to compile expression '" + parameters.at("formula") + "': " + parser.error());
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void MathPluginDecorator<UserType>::doPostRead() {
    _target->postRead();

    // update data pointer
    valueView->rebase(_target->accessChannel(0).data());

    // update parameters
    for(auto& p : params) {
      p.first->read();
      p.second->rebase(p.first->accessChannel(0).data());
    }

    // evaluate the expression, obtain the result in a way so it also works when using the return statement
    double valueWhenNotUsingReturn = expression.value();
    exprtk::results_context<double> results = expression.results();

    if(results.count() == 0) {
      // if results.count() is 0, the return statement presumably has not been used
      if(buffer_2D[0].size() != 1) {
        throw ChimeraTK::logic_error("LogicalNameMapping MathPlugin for register '" + this->getName() +
            "': The expression returns a scalar but " + std::to_string(buffer_2D[0].size()) +
            " expected for read operations.");
      }
      buffer_2D[0][0] = numericToUserType<UserType>(valueWhenNotUsingReturn);
    }
    else if(results.count() == 1) {
      // return statement has been used to return exactly one value (note: this value might be an array)
      exprtk::type_store<double> result = results[0];

      // make sure we got a numeric result
      if(result.type != exprtk::type_store<double>::e_scalar && result.type != exprtk::type_store<double>::e_vector) {
        throw ChimeraTK::logic_error("LogicalNameMapping MathPlugin for register '" + this->getName() +
            "': The expression did not return a numeric result.");
      }

      // create vector view and check that its size matches
      exprtk::type_store<double>::type_view<double> view(result);
      if(view.size() != buffer_2D[0].size()) {
        throw ChimeraTK::logic_error("LogicalNameMapping MathPlugin for register '" + this->getName() +
            "': The expression returns " + std::to_string(view.size()) + " elements but " +
            std::to_string(buffer_2D[0].size()) + " expected for read operations.");
      }

      // convert and copy data into user buffer
      for(size_t k = 0; k < view.size(); ++k) {
        buffer_2D[0][k] = numericToUserType<UserType>(view[k]);
      }
    }
    else {
      // multiple results in return statement are unexpected
      throw ChimeraTK::logic_error("LogicalNameMapping MathPlugin for register '" + this->getName() +
          "': The expression returned " + std::to_string(results.count()) + " results, expect exactly one result.");
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void MathPluginDecorator<UserType>::doPreWrite() {
    // convert from UserType to double - use the target accessor's buffer as a temporary buffer (this is a bit a hack,
    // but it is safe to overwrite the buffer and we can avoid the need for an additional permanent buffer which might
    // not even be used if the register is never written).
    for(size_t k = 0; k < buffer_2D[0].size(); ++k) {
      _target->accessData(0, k) = userTypeToNumeric<double>(buffer_2D[0][k]);
    }

    // inform the value view of the new data pointer - the buffer might have been swapped
    valueView->rebase(_target->accessChannel(0).data());

    // update parameters
    for(auto& p : params) {
      p.first->read();
      p.second->rebase(p.first->accessChannel(0).data());
    }

    // evaluate the expression, obtain the result in a way so it also works when using the return statement
    double valueWhenNotUsingReturn = expression.value();
    exprtk::results_context<double> results = expression.results();
    if(results.count() == 0) {
      // if results.count() is 0, the return statement presumably has not been used
      if(_target->accessChannel(0).size() != 1) {
        throw ChimeraTK::logic_error("LogicalNameMapping MathPlugin for register '" + this->getName() +
            "': The expression returns a scalar but " + std::to_string(buffer_2D[0].size()) +
            " expected for write operations.");
      }
      _target->accessData(0, 0) = valueWhenNotUsingReturn;
    }
    else if(results.count() == 1) {
      // return statement has been used to return exactly one value (note: this value might be an array)
      exprtk::type_store<double> result = results[0];

      // make sure we got a numeric result
      if(result.type != exprtk::type_store<double>::e_scalar && result.type != exprtk::type_store<double>::e_vector) {
        throw ChimeraTK::logic_error("LogicalNameMapping MathPlugin for register '" + this->getName() +
            "': The expression did not return a numeric result.");
      }

      // create vector view and check that its size matches
      exprtk::type_store<double>::type_view<double> view(result);
      if(view.size() != _target->getNumberOfSamples()) {
        throw ChimeraTK::logic_error("LogicalNameMapping MathPlugin for register '" + this->getName() +
            "': The expression returns " + std::to_string(view.size()) + " elements but " +
            std::to_string(_target->getNumberOfSamples()) + " expected for write operations.");
      }

      // convert and copy data into target buffer
      for(size_t k = 0; k < view.size(); ++k) {
        _target->accessData(0, k) = view[k];
      }
    }
    else {
      // multiple results in return statement are unexpected
      throw ChimeraTK::logic_error("LogicalNameMapping MathPlugin for register '" + this->getName() +
          "': The expression returned " + std::to_string(results.count()) + " results, expect exactly one result.");
    }
    _target->preWrite();
  }

  /********************************************************************************************************************/

  /** Helper class to implement MultiplierPlugin::decorateAccessor (can later be realised with if constexpr) */
  template<typename UserType, typename TargetType>
  struct MathPlugin_Helper {
    static boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>&, boost::shared_ptr<NDRegisterAccessor<TargetType>>&,
        const std::map<std::string, std::string>&) {
      assert(false); // only specialisation is valid
      return {};
    }
  };

  /********************************************************************************************************************/

  template<typename UserType>
  struct MathPlugin_Helper<UserType, double> {
    static boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend, boost::shared_ptr<NDRegisterAccessor<double>>& target,
        const std::map<std::string, std::string>& parameters) {
      return boost::make_shared<MathPluginDecorator<UserType>>(backend, target, parameters);
    }
  };

  /********************************************************************************************************************/

  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> MathPlugin::decorateAccessor(
      boost::shared_ptr<LogicalNameMappingBackend>& backend,
      boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) const {
    return MathPlugin_Helper<UserType, TargetType>::decorateAccessor(backend, target, _parameters);
  }

  /********************************************************************************************************************/

}} // namespace ChimeraTK::LNMBackend
