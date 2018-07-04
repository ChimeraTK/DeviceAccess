/*
 *  Generic module to restrict a value into a certain range.
 */

#ifndef CHIMERATK_APPLICATION_CORE_LIMIT_VALUE_H
#define CHIMERATK_APPLICATION_CORE_LIMIT_VALUE_H

#include "ApplicationCore.h"

namespace ChimeraTK {

  template<typename UserType>
  struct LimitValueModuleBase : public ApplicationModule {
      using ApplicationModule::ApplicationModule;

      ScalarPushInput<UserType> input{this, "input", "", "The input value to be limited into the range."};
      ScalarOutput<UserType> output{this, "output", "", "The output value after limiting."};
      ScalarOutput<int> isLimited{this, "isLimited", "", "Boolean set to true if the value was limited and to false otherwise."};

      void applyLimit(UserType min, UserType max) {
        bool wasLimited = isLimited;

        // clamp input value into given range
        UserType value = input;
        if(value > max) {
          output = max;
          isLimited = true;
        }
        else if(value < min) {
          output = min;
          isLimited = true;
        }
        else {
          output = value;
          isLimited = false;
        }

        // write output. isLimited is only written when changed
        output.write();
        if(isLimited != wasLimited) isLimited.write();
        wasLimited = isLimited;

      }

  };

  template<typename UserType>
  struct LimitValue : public LimitValueModuleBase<UserType> {
      using LimitValueModuleBase<UserType>::LimitValueModuleBase;
      ScalarPushInput<UserType> min{this, "min", "", "The minimum allowed value."};
      ScalarPushInput<UserType> max{this, "max", "", "The maximum allowed value."};
      using LimitValueModuleBase<UserType>::input;
      using LimitValueModuleBase<UserType>::applyLimit;

      void mainLoop() {
        auto readGroup = this->readAnyGroup();
        while(true) {
          applyLimit(min,max);
          // wait for new input values (at the end, since we want to process the initial values first)
          readGroup.readAny();
        }
      }
  };


  template<typename UserType, UserType min, UserType max>
  struct FixedLimitValue : public LimitValueModuleBase<UserType> {
      using LimitValueModuleBase<UserType>::LimitValueModuleBase;
      using LimitValueModuleBase<UserType>::input;
      using LimitValueModuleBase<UserType>::applyLimit;

      void mainLoop() {
        while(true) {
          applyLimit(min,max);
          // wait for new input values (at the end, since we want to process the initial values first)
          input.read();
        }
      }

  };

} // namespace ChimeraTK

#endif /* CHIMERATK_APPLICATION_CORE_LIMIT_VALUE_H */
