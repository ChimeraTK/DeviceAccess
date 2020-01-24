#ifndef CHIMERATK_STATUS_AGGREGATOR_H
#define CHIMERATK_STATUS_AGGREGATOR_H

#include "ApplicationModule.h"
#include "ModuleGroup.h"
#include "StatusMonitor.h"

#include <list>
#include <vector>
#include <string>

namespace ChimeraTK{

  /**
   * The StatusAggregator collects results of multiple StatusMonitor instances
   * and aggregates them into a single status, which can take the same values
   * as the result of the individual monitors.
   *
   * Note: The aggregated instances are collected on construction. Hence, the
   * StatusAggregator has to be declared after all instances that shall to be
   * included in the scope (ModuleGroup, Application, ...) of interest.
   */
  class StatusAggregator : public ApplicationModule {
  public:

    StatusAggregator(EntityOwner* owner, const std::string& name, const std::string& description,
                     const std::string& output,
                     HierarchyModifier modifier, const std::unordered_set<std::string>& tags = {})
      : ApplicationModule(owner, name, description, modifier, tags), status(this, output, "", "", {})  {
      populateStatusInput();
    }

    ~StatusAggregator() override {}

  protected:

    void mainLoop() override {

      std::cout << "Entered StatusAggregator::mainLoop()" << std::endl;

//      while(true){
//      // Status collection goes here
//      }
    }

    /// Recursivly search for StatusMonitors and other StatusAggregators
    void populateStatusInput();

    /**One of four possible states to be reported*/
    ScalarOutput<uint16_t> status;

    /**Vector of status inputs */
    std::vector<ScalarPushInput<uint16_t>> statusInput;

    //TODO Also provide this for the aggregator?
//    /** Disable the monitor. The status will always be OFF. You don't have to connect this input.
//     *  When there is no feeder, ApplicationCore will connect it to a constant feeder with value 0, hence the monitor is always enabled.
//     */
//    ScalarPushInput<int> disable{this, "disable", "", "Disable the status monitor"};

  };

} // namespace ChimeraTK
#endif // CHIMERATK_STATUS_AGGREGATOR_H
