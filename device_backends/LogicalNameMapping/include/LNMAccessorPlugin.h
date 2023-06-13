// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "LogicalNameMappingBackend.h"
#include "VirtualFunctionTemplate.h"

#include <utility>

namespace ChimeraTK::LNMBackend {

  /** Helper struct to hold extra parameters needed by some plugins, used in decorateAccessor() */
  struct UndecoratedParams {
    UndecoratedParams(const std::string& name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags)
    : _name(name), _numberOfWords(numberOfWords), _wordOffsetInRegister(wordOffsetInRegister),
      _flags(std::move(flags)) {}
    std::string _name;
    size_t _numberOfWords;
    size_t _wordOffsetInRegister;
    AccessModeFlags _flags;
  };

  /** Base class for AccessorPlugins used by the LogicalNameMapping backend to store backends in lists. When writing
   *  plugins, the class AccessorPlugin should be implemented, not this one. */
  class AccessorPluginBase {
   public:
    explicit AccessorPluginBase(const LNMBackendRegisterInfo& info);
    virtual ~AccessorPluginBase() = default;

    /** Called by the backend when obtaining a register accessor. */
    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getAccessor(boost::shared_ptr<LogicalNameMappingBackend> backend,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags, size_t pluginIndex) {
      return CALL_VIRTUAL_FUNCTION_TEMPLATE(
          getAccessor_impl, UserType, backend, numberOfWords, wordOffsetInRegister, flags, pluginIndex);
    }
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAccessor_impl,
        boost::shared_ptr<NDRegisterAccessor<T>>(
            boost::shared_ptr<LogicalNameMappingBackend>, size_t, size_t, AccessModeFlags, size_t));

    /**
     *  Update the register info inside the catalogue if needed. This function is called by the backend after the
     *  LNMBackendRegisterInfo has
     *  been filled with all information from the target backend.
     *
     *  This function implements the common steps and calls doRegisterInfoUpdate. where the actual implementation is happening.
     */
    void updateRegisterInfo(BackendRegisterCatalogue<LNMBackendRegisterInfo>&);

    /**
     *   Implementation of the plugin specific register information update. Do not call this function directly.
     *   It is always called from updateRegisterInfo()
     *
     *   If plugins intend to change the catalogue information,
     *  they need to do it in this function. This function is only called if the RegisterCatalogue is obtained from the
     *  device, so do not rely on this function to be called.
     *
     *  If the plugin needs data that depends on the target and which is only available after opening (e.g. whether the
     *  register is writeable), the plugin has to call updateRegisterInfo() in the openHook() and can then
     *  modify internal variables in the doRegisterInfoUpdate() function.
     *
     *  Note: in principle it is fine to do nothing in this function, if no catalogue change is required. This function
     *  intentionally has no empty default implementation, because it might otherwise easy to overlook that the register
     *  info must be updated here instead of the constructor.
     */
    virtual void doRegisterInfoUpdate() = 0;

    /**
     *  Hook called when the backend is opened, at the end of the open() function after all backend work has been done
     *  already.
     */
    virtual void openHook(const boost::shared_ptr<LogicalNameMappingBackend>& backend) { std::ignore = backend; }

    /**
     *  Hook called after the parsing of logical name map. This can be used for setup code which needs complete
     *  logical name map definitions but must be executed before any register accessor is created.
     */
    virtual void postParsingHook([[maybe_unused]] const boost::shared_ptr<const LogicalNameMappingBackend>& backend) {}

    /**
     *  Hook called when the backend is closed, at the beginning of the close() function when the device is still open.
     */
    virtual void closeHook() {}

    /**
     *  Hook called when an exception is reported to the the backend via setException(), after the backend has been
     *  moved into the exception state.
     */
    virtual void exceptionHook() {}

   protected:
    /** RegisterInfo describing the the target register for which this plugin instance should work. */
    LNMBackendRegisterInfo _info;
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
    /**
     * The constructor of the plugin should also accept a 3rd argument:
     *   const std::map<std::string, std::string>& parameters
     *  Since the parameters are not used in the base class, they do not need to be passed on.
     */
    explicit AccessorPlugin(const LNMBackendRegisterInfo& info, size_t pluginIndex, bool shareTargetAccessors = false);

   private:
    // we make our destructor private and add Derived as a friend to enforce the correct CRTP
    ~AccessorPlugin() override = default;
    friend Derived;

   protected:
    /**
     * Deriving plugins should set this to true if they want to use interlocked access to the same
     * target accessor. Otherwise different accessors for the same target will given out.
     */
    const bool _needSharedTarget;

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
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target, const UndecoratedParams& accessorParams);

    /** This function is called by the backend. Do not override in implementations. */
    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getAccessor_impl(
        boost::shared_ptr<LogicalNameMappingBackend>& backend, size_t numberOfWords, size_t wordOffsetInRegister,
        AccessModeFlags flags, size_t pluginIndex);

    /**
     * Index of the plugin instance within the stack of plugins on a particular register.
     */
    size_t _pluginIndex;
  };

  /********************************************************************************************************************/

  /** Factory function for accessor plugins */
  boost::shared_ptr<AccessorPluginBase> makePlugin(LNMBackendRegisterInfo info, size_t pluginIndex,
      const std::string& name, const std::map<std::string, std::string>& parameters);

  /********************************************************************************************************************/
  /* Known plugins are defined below (implementations should go to a separate .cc file)                               */
  /********************************************************************************************************************/

  /** Multiplier Plugin: Multiply register's data with a constant factor */
  class MultiplierPlugin : public AccessorPlugin<MultiplierPlugin> {
   public:
    MultiplierPlugin(
        const LNMBackendRegisterInfo& info, size_t pluginIndex, const std::map<std::string, std::string>& parameters);

    void doRegisterInfoUpdate() override;
    DataType getTargetDataType(DataType) const override { return DataType::float64; }

    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target, const UndecoratedParams& accessorParams);

    double _factor;
  };

  /********************************************************************************************************************/

  /** Monostable Trigger Plugin: Write value to target which falls back to another value after defined time. */
  class MonostableTriggerPlugin : public AccessorPlugin<MonostableTriggerPlugin> {
   public:
    MonostableTriggerPlugin(
        LNMBackendRegisterInfo info, size_t pluginIndex, const std::map<std::string, std::string>& parameters);

    void doRegisterInfoUpdate() override;
    DataType getTargetDataType(DataType) const override { return DataType::uint32; }

    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target, const UndecoratedParams& accessorParams);

    double _milliseconds;
    uint32_t _active{1};
    uint32_t _inactive{0};
  };

  /** ForceReadOnly Plugin: Forces a register to be read only. */
  class ForceReadOnlyPlugin : public AccessorPlugin<ForceReadOnlyPlugin> {
   public:
    ForceReadOnlyPlugin(
        const LNMBackendRegisterInfo& info, size_t pluginIndex, const std::map<std::string, std::string>& parameters);

    void doRegisterInfoUpdate() override;

    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target, const UndecoratedParams& accessorParams);
  };

  /** ForcePollingRead Plugin: Forces a register to not allow setting the AccessMode::wait_for_new_data flag. */
  class ForcePollingReadPlugin : public AccessorPlugin<ForcePollingReadPlugin> {
   public:
    ForcePollingReadPlugin(
        const LNMBackendRegisterInfo& info, size_t pluginIndex, const std::map<std::string, std::string>& parameters);

    void doRegisterInfoUpdate() override;

    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target, const UndecoratedParams& accessorParams);
  };

  /** TypeHintModifier Plugin: Change the catalogue type of the mapped register. No actual type conversion takes place */
  class TypeHintModifierPlugin : public AccessorPlugin<TypeHintModifierPlugin> {
   public:
    TypeHintModifierPlugin(
        const LNMBackendRegisterInfo& info, size_t pluginIndex, const std::map<std::string, std::string>& parameters);

    void doRegisterInfoUpdate() override;

   private:
    DataType _dataType{DataType::none};
  };

  class BitRangeAccessPlugin : public AccessorPlugin<BitRangeAccessPlugin> {
   public:
    BitRangeAccessPlugin(
        const LNMBackendRegisterInfo& info, size_t pluginIndex, const std::map<std::string, std::string>& parameters);

    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target, const UndecoratedParams& accessorParams);

    void doRegisterInfoUpdate() override;
    DataType getTargetDataType(DataType /*userType*/) const override { return DataType::uint64; }
    uint32_t _shift{0};
    uint32_t _numberOfBits{0};
    bool _writeable{true};
  };

  /********************************************************************************************************************/
  /* Implementations follow here                                                                                      */
  /********************************************************************************************************************/

  template<typename Derived>
  AccessorPlugin<Derived>::AccessorPlugin(
      const LNMBackendRegisterInfo& info, size_t pluginIndex, bool shareTargetAccessors)
  : AccessorPluginBase(info), _needSharedTarget{shareTargetAccessors}, _pluginIndex(pluginIndex) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAccessor_impl);
  }

  /********************************************************************************************************************/

  template<typename Derived>
  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> AccessorPlugin<Derived>::decorateAccessor(
      boost::shared_ptr<LogicalNameMappingBackend>&, boost::shared_ptr<NDRegisterAccessor<TargetType>>& target,
      const UndecoratedParams&) {
    if constexpr(std::is_same<UserType, TargetType>::value) {
      return target;
    }

    assert(false); // When overriding getTargetDataType(), also decorateAccessor()
                   // must be overridden!

    return {};
  }

  /********************************************************************************************************************/

  template<typename Derived>
  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> AccessorPlugin<Derived>::getAccessor_impl(
      boost::shared_ptr<LogicalNameMappingBackend>& backend, size_t numberOfWords, size_t wordOffsetInRegister,
      AccessModeFlags flags, size_t pluginIndex) {
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorated;

    assert(_pluginIndex == pluginIndex);

    // obtain desired target type from plugin implementation
    auto type = getTargetDataType(typeid(UserType));
    if((_info._dataDescriptor.rawDataType() == DataType::none) && flags.has(AccessMode::raw)) {
      throw ChimeraTK::logic_error(
          "Access mode 'raw' is not supported for register '" + std::string(_info.getRegisterName()) + "'");
    }

    callForType(type, [&](auto T) {
      boost::shared_ptr<ChimeraTK::NDRegisterAccessor<decltype(T)>> target;

      if(_needSharedTarget) {
        auto& map = boost::fusion::at_key<decltype(T)>(backend->sharedAccessorMap.table);
        RegisterPath path{_info.registerName};
        path.setAltSeparator(".");
        LogicalNameMappingBackend::AccessorKey key{backend.get(), path};

        auto it = map.find(key);
        if(it == map.end() || (target = it->second.accessor.lock()) == nullptr) {
          // obtain target accessor with desired type
          target = backend->getRegisterAccessor_impl<decltype(T)>(
              _info.getRegisterName(), numberOfWords, wordOffsetInRegister, flags, pluginIndex + 1);
          map[key].accessor = target;
        }
      }
      else {
        target = backend->getRegisterAccessor_impl<decltype(T)>(
            _info.getRegisterName(), numberOfWords, wordOffsetInRegister, flags, pluginIndex + 1);
      }

      // double buffering plugin needs numberOfWords, wordOffsetInRegister of already existing accessor
      UndecoratedParams accessorParams(_info.registerName, numberOfWords, wordOffsetInRegister, flags);
      decorated = static_cast<Derived*>(this)->template decorateAccessor<UserType>(backend, target, accessorParams);
    });

    decorated->setExceptionBackend(backend);
    return decorated;
  }

} // namespace ChimeraTK::LNMBackend
