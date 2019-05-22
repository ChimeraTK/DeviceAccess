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
    else if(name == "math") {
      return boost::make_shared<MathPlugin>(info, parameters);
    }
    else if(name == "monostableTrigger") {
      return boost::make_shared<MonostableTriggerPlugin>(info, parameters);
    }
    else if(name == "forceReadOnly") {
      return boost::make_shared<ForceReadOnlyPlugin>(info, parameters);
    }
    else if(name == "forcePollingRead") {
      return boost::make_shared<ForcePollingReadPlugin>(info, parameters);
    }
    else {
      throw ChimeraTK::logic_error("LogicalNameMappingBackend: Unknown plugin type '" + name + "'.");
    }
  }

}} // namespace ChimeraTK::LNMBackend
