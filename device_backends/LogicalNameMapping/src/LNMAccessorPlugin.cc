#include <boost/make_shared.hpp>

#include "LNMBackendRegisterInfo.h"
#include "LNMAccessorPlugin.h"
#include "NDRegisterAccessorDecorator.h"

namespace ChimeraTK { namespace LNMBackend {

  /********************************************************************************************************************/

  boost::shared_ptr<AccessorPluginBase> makePlugin(boost::shared_ptr<LNMBackendRegisterInfo> info,
      const std::string& name, const std::map<std::string, std::string>& parameters) {
    if(name == "multiply") {
      return boost::make_shared<MultiplierPlugin>(info, parameters);
    }
    else {
      throw ChimeraTK::logic_error("LogicalNameMappingBackend: Unknown plugin type '" + name + "'.");
    }
  }

  /********************************************************************************************************************/

  template<typename Derived>
  AccessorPlugin<Derived>::AccessorPlugin(boost::shared_ptr<LNMBackendRegisterInfo> info) : _info(info) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAccessor_impl);
  }

  /********************************************************************************************************************/

  template<typename Derived>
  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> AccessorPlugin<Derived>::decorateAccessor(
      boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) const {
    static_assert(std::is_same<UserType, TargetType>(),
        "LogicalNameMapper AccessorPlugin: When overriding getTargetDataType(), also decorateAccessor() must be "
        "overridden!");
    return target;
  }

  /********************************************************************************************************************/

  template<typename Derived>
  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> AccessorPlugin<Derived>::getAccessor_impl(
      LogicalNameMappingBackend& backend, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags,
      size_t pluginIndex) const {
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorated;

    // obtain desired target type from plugin implementation
    auto type = getTargetDataType(typeid(UserType));
    callForType(type, [&](auto T) {
      // obtain target accessor with desired type
      auto target = backend.getRegisterAccessor_impl<decltype(T)>(
          _info->getRegisterName(), numberOfWords, wordOffsetInRegister, flags, pluginIndex + 1);
      decorated = static_cast<const Derived*>(this)->template decorateAccessor<UserType>(target);
    });
    return decorated;
  }

  /********************************************************************************************************************/

  /** Helper function for implementations based on double data type: Convert a double value into the UserType, even if
   *  it is a string. */
  template<typename UserType>
  UserType doubleToUserType(double value) {
    return UserType(value);
  }

  template<>
  std::string doubleToUserType<std::string>(double value) {
    return std::to_string(value);
  }

  /********************************************************************************************************************/

  /** Helper function for implementations based on double data type: Convert a UserType value into double, even if
   *  it is a string. */
  template<typename UserType>
  double userTypeToDouble(const UserType& value) {
    return double(value);
  }

  template<>
  double userTypeToDouble<std::string>(const std::string& value) {
    return std::stod(value);
  }

  /********************************************************************************************************************/
  /* MultiplierPlugin                                                                                                 */
  /********************************************************************************************************************/

  MultiplierPlugin::MultiplierPlugin(
      boost::shared_ptr<LNMBackendRegisterInfo> info, const std::map<std::string, std::string>& parameters)
  : AccessorPlugin(info) {
    // extract parameters
    if(parameters.find("factor") == parameters.end()) {
      throw ChimeraTK::logic_error("LogicalNameMappingBackend MultiplierPlugin: Missing parameter 'factor'.");
    }
    _factor = std::stod(parameters.at("factor"));

    // Change data type to non-integral, if we are multiplying with a non-integer factor
    if(std::abs(_factor - std::round(_factor)) <= std::numeric_limits<double>::epsilon()) {
      auto d = info->_dataDescriptor;
      info->_dataDescriptor = ChimeraTK::RegisterInfo::DataDescriptor(d.fundamentalType(), false, false,
          std::numeric_limits<double>::max_digits10, -std::numeric_limits<double>::min_exponent10, d.rawDataType(),
          d.transportLayerDataType());
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  struct MultiplierPluginDecorator : ChimeraTK::NDRegisterAccessorDecorator<UserType, double> {
    using ChimeraTK::NDRegisterAccessorDecorator<UserType, double>::buffer_2D;

    MultiplierPluginDecorator(const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<double>>& target, double factor)
    : ChimeraTK::NDRegisterAccessorDecorator<UserType, double>(target), _factor(factor) {}

    void doPreRead() override { _target->preRead(); }

    void doPostRead() override;

    void doPreWrite() override;

    void doPostWrite() override { _target->postWrite(); }

    void interrupt() override { _target->interrupt(); }

    ChimeraTK::VersionNumber getVersionNumber() const override { return _target->getVersionNumber(); }

    double _factor;

    using ChimeraTK::NDRegisterAccessorDecorator<UserType, double>::_target;
  };

  template<typename UserType>
  void MultiplierPluginDecorator<UserType>::doPostRead() {
    _target->postRead();
    for(size_t i = 0; i < _target->getNumberOfChannels(); ++i) {
      for(size_t k = 0; k < _target->getNumberOfSamples(); ++k) {
        buffer_2D[i][k] = doubleToUserType<UserType>(_target->accessData(i, k) * _factor);
      }
    }
  }

  template<typename UserType>
  void MultiplierPluginDecorator<UserType>::doPreWrite() {
    for(size_t i = 0; i < _target->getNumberOfChannels(); ++i) {
      for(size_t k = 0; k < _target->getNumberOfSamples(); ++k) {
        _target->accessData(i, k) = userTypeToDouble(buffer_2D[i][k]) * _factor;
      }
    }
    _target->preWrite();
  }

  /********************************************************************************************************************/

  /** Helper class to implement MultiplierPlugin::decorateAccessor (can later be realised with if constexpr) */
  template<typename UserType, typename TargetType>
  struct MultiplierPlugin_Helper {
    static boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<NDRegisterAccessor<TargetType>>&, double) {
      assert(false); // only specialisation is valid
      return {};
    }
  };
  template<typename UserType>
  struct MultiplierPlugin_Helper<UserType, double> {
    static boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<NDRegisterAccessor<double>>& target, double factor) {
      return boost::make_shared<MultiplierPluginDecorator<UserType>>(target, factor);
    }
  };

  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> MultiplierPlugin::decorateAccessor(
      boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) const {
    return MultiplierPlugin_Helper<UserType, TargetType>::decorateAccessor(target, _factor);
  }

}} // namespace ChimeraTK::LNMBackend
