#pragma once

#include "VirtualFunctionTemplate.h"
#include "LogicalNameMappingBackend.h"
#include "ReadAnyGroup.h"

namespace ChimeraTK { namespace LNMBackend {

  /** Base class for AccessorPlugins used by the LogicalNameMapping backend to store backends in lists. When writing
   *  plugins, the class AccessorPlugin should be implemented, not this one. */
  class AccessorPluginBase {
   public:
    virtual ~AccessorPluginBase() {}

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
     *  been filled with all information from the target backend. If plugins intend to change the catalogue information,
     *  they need to do it in this function. This function is only called if the RegisterCatalogue is obtained from the
     *  device, so do not rely on this function to be called.
     *
     *  If the plugin needs data that depends on the target and which is only available after opening (e.g. whether the
     *  register is writeable), the plugin has to call updateRegisterInfo in the openHook() and can then
     *  modify internal vadiables in the updateRegisterInfo function.
     *
     *  Note: in principle it is fine to do nothing in this function, if no catalogue change is required. This function
     *  intentionally has no empty default implementation, because it might otherwise easy to overlook that the register
     *  info must be updated here instead of the constructor.
     */
    virtual void updateRegisterInfo(BackendRegisterCatalogue<LNMBackendRegisterInfo>&) = 0;

    /**
     *  Hook called when the backend is openend, at the end of the open() function after all backend work has been done
     *  already.
     */
    virtual void openHook(boost::shared_ptr<LogicalNameMappingBackend> backend) { std::ignore = backend; }

    /**
     *  Hook called when the backend is closed, at the beginning of the close() function when the device is still open.
     */
    virtual void closeHook() {}

    /**
     *  Hook called when an exception is reported to the the backend via setException(), after the backend has been
     *  moved into the exception state.
     */
    virtual void exceptionHook() {}
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
    AccessorPlugin(LNMBackendRegisterInfo info);

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
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target);

    /** This function is called by the backend. Do not override in implementations. */
    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getAccessor_impl(
        boost::shared_ptr<LogicalNameMappingBackend>& backend, size_t numberOfWords, size_t wordOffsetInRegister,
        AccessModeFlags flags, size_t pluginIndex);

    /** RegisterInfo describing the the target register for which this plugin instance should work. */
    LNMBackendRegisterInfo _info;
  };

  /********************************************************************************************************************/

  /** Factory function for accessor plugins */
  boost::shared_ptr<AccessorPluginBase> makePlugin(
      LNMBackendRegisterInfo info, const std::string& name, const std::map<std::string, std::string>& parameters);

  /********************************************************************************************************************/
  /* Known plugins are defined below (implementations should go to a separate .cc file)                               */
  /********************************************************************************************************************/

  /** Multiplier Plugin: Multiply register's data with a constant factor */
  class MultiplierPlugin : public AccessorPlugin<MultiplierPlugin> {
   public:
    MultiplierPlugin(LNMBackendRegisterInfo info, const std::map<std::string, std::string>& parameters);

    void updateRegisterInfo(BackendRegisterCatalogue<LNMBackendRegisterInfo>&) override;
    DataType getTargetDataType(DataType) const override { return DataType::float64; }

    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target);

    double _factor;
  };

  // forward declaration needed for MathPlugin
  struct MathPluginFormulaHelper;

  /** Math Plugin: Apply mathematical formula to register's data. The formula is parsed by the exprtk library. */
  class MathPlugin : public AccessorPlugin<MathPlugin> {
   public:
    MathPlugin(LNMBackendRegisterInfo info, const std::map<std::string, std::string>& parameters);

    void updateRegisterInfo(BackendRegisterCatalogue<LNMBackendRegisterInfo>&) override;
    DataType getTargetDataType(DataType) const override { return DataType::float64; }

    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target);

    void openHook(boost::shared_ptr<LogicalNameMappingBackend> backend) override;
    void closeHook() override;
    void exceptionHook() override;

    // This function is starting a loop and will be executed in the _parameterThread;
    void parameterReadLoop(boost::barrier& waitUntilThreadLaunched);

    bool _isWrite{false};
    bool _hasPushParameter{false};           // can only be true if _isWrite == true
    bool _mainValueWrittenAfterOpen{false};  // only needed if _hasPushParameter == true
    boost::thread _pushParameterWriteThread; // only used if _hasPushParameter == true
    ReadAnyGroup _pushParameterReadGroup;    // only used if _hasPushParameter == true
    std::map<std::string, boost::shared_ptr<NDRegisterAccessor<double>>>
        _pushParameterAccessorMap;                 // only used if _hasPushParameter == true
    boost::shared_ptr<MathPluginFormulaHelper> _h; // only used if _hasPushParameter == true
    std::vector<double> _lastWrittenValue;         // only used if _hasPushParameter == true
    // The _writeMutex has two functions:
    // - It protects resources which are share by an accesor and the parameter thread
    //   (Currently: _lastWrittenValue and _mainValueWrittenAfterOpen)
    // - It is held while an accessor or the parameter thread is doing the preWrite/writeTransfer/postWrite sequence.
    //   If the other thread would be able to do a transfer bwetween the preWrite and the actual transfer this
    //   would lead to wrong results (although formally the code is thread safe)
    // Use a recursive mutex because it is allowed to call preWrite() multiple times before executing
    // the writeTransfer, and the mutex is accquired in preWrite() and release in only in postWrite().
    std::recursive_mutex _writeMutex;                    // only used if _hasPushParameter == true
    boost::weak_ptr<LogicalNameMappingBackend> _backend; // set in openHook()

    std::map<std::string, std::string> _parameters;
    std::string _formula;              // extracted from _parameters
    bool _enablePushParameters{false}; // extracted from _parameters

    // Checks that all parameters have been written since opening the device.
    // Returns false as long as at least one parameter is still on the backend's _versionOnOpen.
    bool checkAllParametersWritten(
        std::map<std::string, boost::shared_ptr<NDRegisterAccessor<double>>> const& accessorsMap);
  };

  /** Monostable Trigger Plugin: Write value to target which falls back to another value after defined time. */
  class MonostableTriggerPlugin : public AccessorPlugin<MonostableTriggerPlugin> {
   public:
    MonostableTriggerPlugin(LNMBackendRegisterInfo info, const std::map<std::string, std::string>& parameters);

    void updateRegisterInfo(BackendRegisterCatalogue<LNMBackendRegisterInfo>&) override;
    DataType getTargetDataType(DataType) const override { return DataType::uint32; }

    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target);

    double _milliseconds;
    uint32_t _active{1};
    uint32_t _inactive{0};
  };

  /** ForceReadOnly Plugin: Forces a register to be read only. */
  class ForceReadOnlyPlugin : public AccessorPlugin<ForceReadOnlyPlugin> {
   public:
    ForceReadOnlyPlugin(LNMBackendRegisterInfo info, const std::map<std::string, std::string>& parameters);

    void updateRegisterInfo(BackendRegisterCatalogue<LNMBackendRegisterInfo>&) override;

    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target);
  };

  /** ForcePollingRead Plugin: Forces a register to not allow setting the AccessMode::wait_for_new_data flag. */
  class ForcePollingReadPlugin : public AccessorPlugin<ForcePollingReadPlugin> {
   public:
    ForcePollingReadPlugin(LNMBackendRegisterInfo info, const std::map<std::string, std::string>& parameters);

    void updateRegisterInfo(BackendRegisterCatalogue<LNMBackendRegisterInfo>&) override;

    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target);
  };

  /** TypeHintModifier Plugin: Change the catalog type of the mapped register. No actual type conversion takes place */
  class TypeHintModifierPlugin : public AccessorPlugin<TypeHintModifierPlugin> {
   public:
    TypeHintModifierPlugin(LNMBackendRegisterInfo info, const std::map<std::string, std::string>& parameters);

    void updateRegisterInfo(BackendRegisterCatalogue<LNMBackendRegisterInfo>&) override;

   private:
    DataType _dataType{DataType::none};
  };

  /********************************************************************************************************************/
  /* Implementations follow here                                                                                      */
  /********************************************************************************************************************/

  template<typename Derived>
  AccessorPlugin<Derived>::AccessorPlugin(LNMBackendRegisterInfo info) : _info(info) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAccessor_impl);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetType>
  struct AccessorPlugin_Helper {
    static boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<NDRegisterAccessor<TargetType>>&) {
      assert(false); // When overriding getTargetDataType(), also decorateAccessor()
                     // must be overridden!
      return {};
    }
  };

  template<typename UserType>
  struct AccessorPlugin_Helper<UserType, UserType> {
    static boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<NDRegisterAccessor<UserType>>& target) {
      return target;
    }
  };

  template<typename Derived>
  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> AccessorPlugin<Derived>::decorateAccessor(
      boost::shared_ptr<LogicalNameMappingBackend>&, boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) {
    return AccessorPlugin_Helper<UserType, TargetType>::decorateAccessor(target);
  }

  /********************************************************************************************************************/

  template<typename Derived>
  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> AccessorPlugin<Derived>::getAccessor_impl(
      boost::shared_ptr<LogicalNameMappingBackend>& backend, size_t numberOfWords, size_t wordOffsetInRegister,
      AccessModeFlags flags, size_t pluginIndex) {
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorated;

    // obtain desired target type from plugin implementation
    auto type = getTargetDataType(typeid(UserType));
    if((_info._dataDescriptor.rawDataType() == DataType::none) and flags.has(AccessMode::raw)) {
      throw ChimeraTK::logic_error(
          "Access mode 'raw' is not supported for register '" + std::string(_info.getRegisterName()) + "'");
    }

    callForType(type, [&](auto T) {
      // obtain target accessor with desired type
      auto target = backend->getRegisterAccessor_impl<decltype(T)>(
          _info.getRegisterName(), numberOfWords, wordOffsetInRegister, flags, pluginIndex + 1);
      decorated = static_cast<Derived*>(this)->template decorateAccessor<UserType>(backend, target);
    });

    decorated->setExceptionBackend(backend);
    return decorated;
  }

}} // namespace ChimeraTK::LNMBackend
