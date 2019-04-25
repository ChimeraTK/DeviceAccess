#ifndef CHIMERA_TK_LNM_ACCESSOR_PLUGIN_H
#define CHIMERA_TK_LNM_ACCESSOR_PLUGIN_H

#include "VirtualFunctionTemplate.h"
#include "LogicalNameMappingBackend.h"

namespace ChimeraTK { namespace LNMBackend {

  /** Base class for AccessorPlugins used by the LogicalNameMapping backend to store backends in lists. When writing
   *  plugins, the class AccessorPlugin should be implemented, not this one. */
  class AccessorPluginBase {
   public:
    virtual ~AccessorPluginBase() {}

    /** Called by the backend when obtaining a register accessor. */
    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getAccessor(boost::shared_ptr<LogicalNameMappingBackend> backend,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags, size_t pluginIndex) const {
      return CALL_VIRTUAL_FUNCTION_TEMPLATE(
          getAccessor_impl, UserType, backend, numberOfWords, wordOffsetInRegister, flags, pluginIndex);
    }
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAccessor_impl,
        boost::shared_ptr<NDRegisterAccessor<T>>(
            boost::shared_ptr<LogicalNameMappingBackend>, size_t, size_t, AccessModeFlags, size_t));

    /**
     *  Update the register info if needed. This function is called by the backend after the LNMBackendRegisterInfo has
     *  been filled with all information from the target backend. If plugins intend to change the catalogue information,
     *  they need to do it in this function. This function is only called if the RegisterCatalogue is obtained from the
     *  device, so do not rely on this function to be called.
     *
     *  Note: in principle it is fine to do nothing in this function, if no catalogue change is required. This function
     *  intentionally has no empty default implementation, because it might otherwise easy to overlook that the register
     *  info must be updated here instead of the constructor.
     */
    virtual void updateRegisterInfo() = 0;
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

   private:
    // we make our destructor private and add Derived as a friend to enforce the correct CRTP
    virtual ~AccessorPlugin() {}
    friend Derived;

   public:
    /**
     *  Return the data type for which the target accessor shall be obtained. By default the same type as the requested
     *  data type by the user is used. By overriding this function, plugins can change this. E.g. plugins implementing
     *  numeric calculations will typically always request their target accessor with UserType=double, so they should
     *  always return DataType::float64 here.
     */
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
        boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) const;

    /** This function is called by the backend. Do not override in implementations. */
    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getAccessor_impl(
        boost::shared_ptr<LogicalNameMappingBackend>& backend, size_t numberOfWords, size_t wordOffsetInRegister,
        AccessModeFlags flags, size_t pluginIndex) const;

    /** RegisterInfo describing the the target register for which this plugin instance should work. */
    boost::shared_ptr<LNMBackendRegisterInfo> _info;
  };

  /********************************************************************************************************************/

  /** Factory function for accessor plugins */
  boost::shared_ptr<AccessorPluginBase> makePlugin(boost::shared_ptr<LNMBackendRegisterInfo> info,
      const std::string& name, const std::map<std::string, std::string>& parameters);

  /********************************************************************************************************************/
  /* Known plugins are defined below (implementations should go to a separate .cc file)                               */
  /********************************************************************************************************************/

  /** Multiplier Plugin: Multiply register's data with a constant factor */
  class MultiplierPlugin : public AccessorPlugin<MultiplierPlugin> {
   public:
    MultiplierPlugin(
        boost::shared_ptr<LNMBackendRegisterInfo> info, const std::map<std::string, std::string>& parameters);

    void updateRegisterInfo() override;
    DataType getTargetDataType(DataType) const override { return DataType::float64; }

    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) const;

    double _factor;
  };

  /** Math Plugin: Apply mathematical formula to register's data. The formula is parsed by the exprtk library. */
  class MathPlugin : public AccessorPlugin<MathPlugin> {
   public:
    MathPlugin(boost::shared_ptr<LNMBackendRegisterInfo> info, const std::map<std::string, std::string>& parameters);

    void updateRegisterInfo() override;
    DataType getTargetDataType(DataType) const override { return DataType::float64; }

    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) const;

    std::map<std::string, std::string> _parameters;
  };

  /** Monostable Trigger Plugin: Write value to target which falls back to another value after defined time. */
  class MonostableTriggerPlugin : public AccessorPlugin<MonostableTriggerPlugin> {
   public:
    MonostableTriggerPlugin(
        boost::shared_ptr<LNMBackendRegisterInfo> info, const std::map<std::string, std::string>& parameters);

    void updateRegisterInfo() override;
    DataType getTargetDataType(DataType) const { return DataType::uint32; }

    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) const;

    double _milliseconds;
    uint32_t _active{1};
    uint32_t _inactive{0};
  };

  /** ForceReadOnly Plugin: Forces a register to be read only. */
  class ForceReadOnlyPlugin : public AccessorPlugin<ForceReadOnlyPlugin> {
   public:
    ForceReadOnlyPlugin(
        boost::shared_ptr<LNMBackendRegisterInfo> info, const std::map<std::string, std::string>& parameters);

    void updateRegisterInfo() override;

    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) const;
  };

  /** ForcePollingRead Plugin: Forces a register to not allow setting the AccessMode::wait_for_new_data flag. */
  class ForcePollingReadPlugin : public AccessorPlugin<ForcePollingReadPlugin> {
   public:
    ForcePollingReadPlugin(
        boost::shared_ptr<LNMBackendRegisterInfo> info, const std::map<std::string, std::string>& parameters);

    void updateRegisterInfo() override;

    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) const;
  };

  /********************************************************************************************************************/
  /* Implementations follow here                                                                                      */
  /********************************************************************************************************************/

  template<typename Derived>
  AccessorPlugin<Derived>::AccessorPlugin(boost::shared_ptr<LNMBackendRegisterInfo> info) : _info(info) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAccessor_impl);
  }

  /********************************************************************************************************************/

  template<typename Derived>
  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> AccessorPlugin<Derived>::decorateAccessor(
      boost::shared_ptr<LogicalNameMappingBackend>&, boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) const {
    static_assert(std::is_same<UserType, TargetType>(),
        "LogicalNameMapper AccessorPlugin: When overriding getTargetDataType(), also decorateAccessor() must be "
        "overridden!");
    return target;
  }

  /********************************************************************************************************************/

  template<typename Derived>
  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> AccessorPlugin<Derived>::getAccessor_impl(
      boost::shared_ptr<LogicalNameMappingBackend>& backend, size_t numberOfWords, size_t wordOffsetInRegister,
      AccessModeFlags flags, size_t pluginIndex) const {
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorated;

    // obtain desired target type from plugin implementation
    auto type = getTargetDataType(typeid(UserType));
    callForType(type, [&](auto T) {
      // obtain target accessor with desired type
      auto target = backend->getRegisterAccessor_impl<decltype(T)>(
          _info->getRegisterName(), numberOfWords, wordOffsetInRegister, flags, pluginIndex + 1);
      decorated = static_cast<const Derived*>(this)->template decorateAccessor<UserType>(backend, target);
    });
    return decorated;
  }

}} // namespace ChimeraTK::LNMBackend

#endif // CHIMERA_TK_LNM_ACCESSOR_PLUGIN_H
