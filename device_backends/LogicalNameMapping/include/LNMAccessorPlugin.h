#ifndef CHIMERA_TK_LNM_ACCESSOR_PLUGIN_H
#define CHIMERA_TK_LNM_ACCESSOR_PLUGIN_H

#include "VirtualFunctionTemplate.h"
#include "LogicalNameMappingBackend.h"

namespace ChimeraTK { namespace LNMBackend {

  /** Base class for AccessorPlugins used by the LogicalNameMapping backend to store backends in lists. When writing
   *  plugins, the class AccessorPlugin should be implemented, not this one. */
  class AccessorPluginBase {
   public:
    /** Called by the backend when obtaining a register accessor. */
    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getAccessor(LogicalNameMappingBackend& backend,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags, size_t pluginIndex) const {
      return CALL_VIRTUAL_FUNCTION_TEMPLATE(
          getAccessor_impl, UserType, backend, numberOfWords, wordOffsetInRegister, flags, pluginIndex);
    }
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAccessor_impl,
        boost::shared_ptr<NDRegisterAccessor<T>>(LogicalNameMappingBackend&, size_t, size_t, AccessModeFlags, size_t));
  };

  /** Base class for plugins that modify the behaviour of accessors in the logical name mapping backend. Plugins need to
   *  implement this class. When adding new plugins, the makePlugin() function needs to be modified to create the
   *  plugins when requested. Note that plugins are not user-providable. Plugins can only be added as part of the
   *  DeviceAccess library.
   *
   *  Note: This class implements a CRTP (curiously recurring template pattern). The template argument must be the
   *        derived class implementing the plugin. */
  template<typename Derived>
  class AccessorPlugin : public AccessorPluginBase {
   public:
    /** The constructor of the plugin should also accept a second argument:
     *   const std::map<std::string, std::string>& parameters
     *  Since the parameters are not used in the base class, they do not need to be passed on. */
    AccessorPlugin(boost::shared_ptr<LNMBackendRegisterInfo> info);

    virtual ~AccessorPlugin() {}

    /** Return the data type for which the target accessor shall be obtained. By default the same type as the requested
     *  data type by the user is used. By overriding this function, plugins can change this. E.g. plugins implementing
     *  numeric calculations will typically always request their target accessor with UserType=double, so they should
     *  always return DataType::float64 here. */
    virtual DataType getTargetDataType(DataType userType) const { return userType; }

    /**
     *  This function should be overridden by the plugin (yes, this is possible due to the CRTP). It allows the plugin
     *  to decorate the accessor to change the its behaviour.
     *
     *  Note: Even if getTargetDataType() is overridden, the function will be instantiated for all target types, but
     *        if will be only called for those getTargetDataType() returns.
     */
    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) const;

    /** This function is called by the backend. Do not override in implementations. */
    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getAccessor_impl(LogicalNameMappingBackend& backend,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags, size_t pluginIndex) const;

    /** RegisterInfo describing the the target register for which this plugin instance should work. */
    boost::shared_ptr<LNMBackendRegisterInfo> _info;
  };

  /********************************************************************************************************************/

  /** Factory function for accessor plugins */
  boost::shared_ptr<AccessorPluginBase> makePlugin(boost::shared_ptr<LNMBackendRegisterInfo> info,
      const std::string& name, const std::map<std::string, std::string>& parameters);

  /********************************************************************************************************************/
  /* Known plugins are defined below (implementations must go to .cc file)                                            */
  /********************************************************************************************************************/

  /** Multiplier Plugin: Multiply register's data with a constant factor */
  class MultiplierPlugin : public AccessorPlugin<MultiplierPlugin> {
   public:
    MultiplierPlugin(
        boost::shared_ptr<LNMBackendRegisterInfo> info, const std::map<std::string, std::string>& parameters);

    virtual DataType getTargetDataType(DataType) const { return DataType::float64; }

    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) const;

    double _factor;
  };

  /** Math Plugin: Apply mathematical formula to register's data. The formula is parsed by the exprtk library. */
  class MathPlugin : public AccessorPlugin<MathPlugin> {
   public:
    MathPlugin(boost::shared_ptr<LNMBackendRegisterInfo> info, const std::map<std::string, std::string>& parameters);

    virtual DataType getTargetDataType(DataType) const { return DataType::float64; }

    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) const;

    std::map<std::string, std::string> _parameters;
  };

}} // namespace ChimeraTK::LNMBackend

#endif // CHIMERA_TK_LNM_ACCESSOR_PLUGIN_H
