#ifndef CHIMERATK_APPLICATION_CORE_PERIODIC_TRIGGER_H
#define CHIMERATK_APPLICATION_CORE_PERIODIC_TRIGGER_H

#include "ApplicationCore.h"

#include <chrono>

namespace ChimeraTK {

  /**
   * Simple periodic trigger that fires a variable once per second.
   * After configurable number of seconds it will wrap around
   */
  struct PeriodicTrigger : public ApplicationModule {
    /** Constructor. In addition to the usual arguments of an ApplicationModule,
     * the default timeout value is specified.
     *  This value is used as a timeout if the timeout value is set to 0. The
     * timeout value is in milliseconds. */
    PeriodicTrigger(EntityOwner* owner, const std::string& name, const std::string& description,
        const uint32_t defaultPeriod = 1000, bool eliminateHierarchy = false,
        const std::unordered_set<std::string>& tags = {})
    : ApplicationModule(owner, name, description, eliminateHierarchy, tags), defaultPeriod_(defaultPeriod) {}

    ScalarPollInput<uint32_t> period{this, "period", "ms",
        "period in milliseconds. The trigger is "
        "sent once per the specified duration."};
    ScalarOutput<uint64_t> tick{this, "tick", "", "Timer tick. Counts the trigger number starting from 0."};

    void prepare() override {
      setCurrentVersionNumber({});
      tick.write(); // send initial value
    }

    void sendTrigger() {
      setCurrentVersionNumber({});
      ++tick;
      tick.write();
    }

    void mainLoop() override {
      if(Application::getInstance().isTestableModeEnabled()) {
        return;
      }
      tick = 0;
      std::chrono::time_point<std::chrono::steady_clock> t = std::chrono::steady_clock::now();

      while(true) {
        period.read();
        if(period == 0) {
          // set receiving end of timeout. Will only be overwritten if there is
          // new data.
          period = defaultPeriod_;
        }
        t += std::chrono::milliseconds(static_cast<uint32_t>(period));
        boost::this_thread::interruption_point();
        std::this_thread::sleep_until(t);

        sendTrigger();
      }
    }

   private:
    uint32_t defaultPeriod_;
  };
} // namespace ChimeraTK

#endif // CHIMERATK_APPLICATION_CORE_PERIODIC_TRIGGER_H
