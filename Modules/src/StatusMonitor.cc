#include "StatusMonitor.h"
namespace ChimeraTK {
  /*StatusMonitor::StatusMonitor(EntityOwner* owner, const std::string& name, 
        const std::string& description, ChimeraTK::HierarchyModifier modifier,
        const std::string& input,
        const std::string& output,
        const std::unordered_set<std::string>& inputTags,
        const std::unordered_set<std::string>& parameterTags,
        const std::unordered_set<std::string>& outputTags)
        : ChimeraTK::ApplicationModule(owner, name, description, modifier),
          _parameterTags(parameterTags), watch(this, input, "", "", inputTags),
          status(this, output,"","",outputTags)
  {
    
  }*/



   StatusMonitor::StatusMonitor(EntityOwner* owner, const std::string& name,
        const std::string& description, ChimeraTK::HierarchyModifier modifier,
        const std::string& input,
        const std::string& output,
        const std::unordered_set<std::string>& defaultTags)
        : ChimeraTK::ApplicationModule(owner, name, description, modifier),
          _parameterTags(defaultTags), watch(this, input, "", "", defaultTags),
          status(this, output,"","",defaultTags)
  {
  }
}
