#ifndef CHIMERATK_APPLICATION_CORE_DATA_LOSS_COUNTER_H
#define CHIMERATK_APPLICATION_CORE_DATA_LOSS_COUNTER_H

#include <map>

#include <ChimeraTK/SupportedUserTypes.h>

#include "ApplicationCore.h"

namespace ChimeraTK {

/**
 *  Module which gathers statistics on data loss inside the application. It will
 * read the data loss counter once per trigger and update the output statistics
 * variables.
 */
struct DataLossCounter : ApplicationModule {

  using ApplicationModule::ApplicationModule;

  ScalarPushInput<int32_t> trigger{this, "trigger", "", "Trigger input"};

  ScalarOutput<uint64_t> lostDataInLastTrigger{
      this, "lostDataInLastTrigger", "",
      "Number of data transfers during "
      "the last trigger which resulted in data loss."};
  ScalarOutput<uint64_t> triggersWithDataLoss{
      this, "triggersWithDataLoss", "",
      "Number of trigger periods during "
      "which at least on data transfer resulted in data loss."};

  void mainLoop() override {
    while (true) {
      trigger.read();
      uint64_t counter = Application::getAndResetDataLossCounter();
      lostDataInLastTrigger = counter;
      if (counter > 0)
        ++triggersWithDataLoss;
      writeAll();
    }
  }
};

} // namespace ChimeraTK

#endif /* CHIMERATK_APPLICATION_CORE_DATA_LOSS_COUNTER_H */
