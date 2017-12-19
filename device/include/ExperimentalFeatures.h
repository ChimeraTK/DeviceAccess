/*
 * ExperimentalFeatures.h - Flag to enabled experimental features
 *
 *  Created on: Feb 3, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_EXPERIMENTAL_FEATURES_H
#define CHIMERATK_EXPERIMENTAL_FEATURES_H

#include <string>
#include <iostream>
#include <map>
#include <mutex>
#include <atomic>

namespace ChimeraTK {

  /** Class for handling the experimental feature enable flag */
  class ExperimentalFeatures {

    public:

      /** Enable experimental features. Call this function in your application if you want to use experimental
       *  features. Beware that your application is likely to break due to incompatible changes in those
       *  features! */
      static void enable() {
        if(isEnabled) return;
        std::cerr << "*******************************************************************************" << std::endl;
        std::cerr << " Experimental features are now enabled in ChimeraTK DeviceAccess" << std::endl;
        std::cerr << "*******************************************************************************" << std::endl;
        isEnabled = true;
      }

      /** Check if experimental features are enabled. If not, the application is terminated with an error message
       *  which will contain the given name of the experimental feature the application was trying to use.
       *  Call this function in the experimental code section of the library to protect it against use without
       *  enbaled experimental features. */
      static void check(const std::string& featureName) {
        if(!isEnabled) {
          std::cerr << "You are using the experimental feature '" << featureName
                    << "' but do not have experimental features enabled!" << std::endl;
          std::terminate();
        }
        std::lock_guard<std::mutex> guard(reminder.lock_useCount);
        reminder.useCount[featureName]++;
      }

    private:

      static std::atomic<bool> isEnabled;

      class Reminder {
        friend class ExperimentalFeatures;
        Reminder() {}
        ~Reminder() {
          if(ExperimentalFeatures::isEnabled) {
            std::cerr << "*******************************************************************************" << std::endl;
            std::cerr << " Experimental features were enabled in ChimeraTK DeviceAccess" << std::endl;
            std::cerr << " The following features were used (use count):" << std::endl;
            for(auto &uc : useCount) {
              std::cerr << "  - " << uc.first << " (" << uc.second << ")" << std::endl;
            }
            std::cerr << "*******************************************************************************" << std::endl;
          }
        }
        std::map<std::string, uint64_t> useCount;
        std::mutex lock_useCount;
      };
      static Reminder reminder;

  };
}

#endif /* CHIMERATK_EXPERIMENTAL_FEATURES_H */
