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
    using ApplicationModule::ApplicationModule;

    ScalarPollInput<int> timeout{this, "timeout", "s", "Timeout in seconds"};
    ScalarOutput<int> tick{this, "tick", "", "Timer tick"};

    void mainLoop()
    {
        tick = 0;
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            tick++;
            tick %= timeout;
            tick.write();
        }
    }
};
}

#endif // CHIMERATK_APPLICATION_CORE_PERIODIC_TRIGGER_H
