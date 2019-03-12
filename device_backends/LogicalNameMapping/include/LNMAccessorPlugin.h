#ifndef CHIMERA_TK_LNM_ACCESSOR_PLUGIN_H
#define CHIMERA_TK_LNM_ACCESSOR_PLUGIN_H

#include "VirtualFunctionTemplate.h"
#include "LogicalNameMappingBackend.h"

namespace ChimeraTK { namespace LNMBackend {

  /** Base class for plugins that modify the behaviour of accessors in the logical name mapping backend. */
  class AccessorPlugin {
   public:
    AccessorPlugin(boost::shared_ptr<LNMBackendRegisterInfo> info) : _info(info) {}

    virtual ~AccessorPlugin() {}

    /** Called by the backend when obtaining a register accessor. This allows the plugin to decorate the
     *  accessor to change the its behaviour. */
    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getAccessor(LogicalNameMappingBackend& backend,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) const {
      return CALL_VIRTUAL_FUNCTION_TEMPLATE(
          getAccessor_impl, UserType, backend, numberOfWords, wordOffsetInRegister, flags);
    }
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAccessor_impl,
        boost::shared_ptr<NDRegisterAccessor<T>>(LogicalNameMappingBackend&, size_t, size_t, AccessModeFlags));

    boost::shared_ptr<LNMBackendRegisterInfo> _info;
  };

  /********************************************************************************************************************/

  /** Factory function for accessor plugins */
  boost::shared_ptr<AccessorPlugin> makePlugin(boost::shared_ptr<LNMBackendRegisterInfo> info, const std::string& name,
      const std::map<std::string, std::string>& parameters);

  /********************************************************************************************************************/
  /* Known plugins are defined below                                                                                  */
  /********************************************************************************************************************/

  class MultiplierPlugin : public AccessorPlugin {
   public:
    MultiplierPlugin(
        boost::shared_ptr<LNMBackendRegisterInfo> info, const std::map<std::string, std::string>& parameters);

    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getAccessor_impl(LogicalNameMappingBackend& backend,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) const;

    double _factor;
  };

}} // namespace ChimeraTK::LNMBackend

#endif // CHIMERA_TK_LNM_ACCESSOR_PLUGIN_H
