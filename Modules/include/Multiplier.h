/*
 *  Generic module to multiply one value with another
 */

#ifndef CHIMERATK_APPLICATION_CORE_MULTIPLIER_H
#define CHIMERATK_APPLICATION_CORE_MULTIPLIER_H

#include <limits>
#include <cmath>

#include "ApplicationCore.h"

namespace ChimeraTK {

  template<typename InputType, typename OutputType=InputType, size_t NELEMS=1>
  struct ConstMultiplier : public ApplicationModule {

      ConstMultiplier(EntityOwner *owner, const std::string &name, const std::string &description, double factor)
      : ApplicationModule(owner, name, description), _factor(factor) {
        setEliminateHierarchy();
      }

      ArrayPushInput<InputType> input{this, "input", "", NELEMS, "Input value to be scaled"};
      ArrayOutput<OutputType> output{this, "output", "", NELEMS, "Output value after scaling"};

      double _factor;

      void mainLoop() {
        while(true) {

          // scale value (with rounding, if integral type)
          if(!std::numeric_limits<OutputType>::is_integer) {
            for(size_t i=0; i<NELEMS; ++i) output[i] = input[i] * _factor;
          }
          else {
            for(size_t i=0; i<NELEMS; ++i) output[i] = std::round(input[i] * _factor);
          }

          // write scaled value
          output.write();

          // wait for new input value (at the end, since we want to process the initial values first)
          input.read();
        }
      }

  };

  template<typename InputType, typename OutputType=InputType, size_t NELEMS=1>
  struct Multiplier : public ApplicationModule {

      using ApplicationModule::ApplicationModule;
      Multiplier(EntityOwner *owner, const std::string &name, const std::string &description)
      : ApplicationModule(owner, name, description) {
        setEliminateHierarchy();
      }

      ArrayPushInput<InputType> input{this, "input", "", NELEMS, "Input value to be scaled"};
      ScalarPushInput<double> factor{this, "factor", "", "Factor to scale the input value with"};
      ArrayOutput<OutputType> output{this, "output", "", NELEMS, "Output value after scaling"};

      void mainLoop() {
        ReadAnyGroup group{input, factor};
        while(true) {

          // scale value (with rounding, if integral type)
          if(!std::numeric_limits<OutputType>::is_integer) {
            for(size_t i=0; i<NELEMS; ++i) output[i] = input[i] * factor;
          }
          else {
            for(size_t i=0; i<NELEMS; ++i) output[i] = std::round(input[i] * factor);
          }

          // write scaled value
          output.write();

          // wait for new input value (at the end, since we want to process the initial values first)
          group.readAny();
        }
      }

  };

  template<typename InputType, typename OutputType=InputType, size_t NELEMS=1>
  struct Divider : public ApplicationModule {

      using ApplicationModule::ApplicationModule;
      Divider(EntityOwner *owner, const std::string &name, const std::string &description)
      : ApplicationModule(owner, name, description) {
        setEliminateHierarchy();
      }

      ArrayPushInput<InputType> input{this, "input", "", NELEMS, "Input value to be scaled"};
      ScalarPushInput<double> divider{this, "divider", "", "Divider to scale the input value with"};
      ArrayOutput<OutputType> output{this, "output", "", NELEMS, "Output value after scaling"};

      void mainLoop() {
        ReadAnyGroup group{input, divider};
        while(true) {

          // scale value (with rounding, if integral type)
          if(!std::numeric_limits<OutputType>::is_integer) {
            for(size_t i=0; i<NELEMS; ++i) output[i] = input[i] / divider;
          }
          else {
            for(size_t i=0; i<NELEMS; ++i) output[i] = std::round(input[i] / divider);
          }

          // write scaled value
          output.write();

          // wait for new input value (at the end, since we want to process the initial values first)
          group.readAny();
        }
      }

  };

} // namespace ChimeraTK

#endif /* CHIMERATK_APPLICATION_CORE_MULTIPLIER_H */
