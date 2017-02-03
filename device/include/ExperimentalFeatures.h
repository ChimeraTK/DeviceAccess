/*
 * ExperimentalFeatures.h - Flag to enabled experimental features
 *
 *  Created on: Feb 3, 2017
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_EXPERIMENTAL_FEATURES_H
#define MTCA4U_EXPERIMENTAL_FEATURES_H

#include <string>
#include <iostream>

namespace ChimeraTK {
  
  /** Class for handling the experimental feature enable flag */
  class ExperimentalFeatures {

    public:
      
      /** Enable experimental features. Call this function in your application if you want to use experimental
       *  features. Beware that your application is likely to break due to incompatible changes in those
       *  features! */
      static void enable() {
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
      }

    private:

      static bool isEnabled;
  };
}

#endif /* MTCA4U_EXPERIMENTAL_FEATURES_H */
