/*
 *  Generic module to multiply one value with another
 */

#ifndef CHIMERATK_APPLICATION_CORE_MULTIPLIER_H
#define CHIMERATK_APPLICATION_CORE_MULTIPLIER_H

#include <cmath>
#include <limits>

#include "ApplicationCore.h"

namespace ChimeraTK {

template <typename InputType, typename OutputType = InputType,
          size_t NELEMS = 1>
struct ConstMultiplier : public ApplicationModule {

  ConstMultiplier(EntityOwner *owner, const std::string &name,
                  const std::string &description, double factor)
      : ApplicationModule(owner, name, ""),
        input(this, "input", "", NELEMS, description),
        output(this, "output", "", NELEMS, description), _factor(factor) {
    setEliminateHierarchy();
  }

  ArrayPushInput<InputType> input;
  ArrayOutput<OutputType> output;

  double _factor;

  void mainLoop() {
    while (true) {

      // scale value (with rounding, if integral type)
      if (!std::numeric_limits<OutputType>::is_integer) {
        for (size_t i = 0; i < NELEMS; ++i)
          output[i] = input[i] * _factor;
      } else {
        for (size_t i = 0; i < NELEMS; ++i)
          output[i] = std::round(input[i] * _factor);
      }

      // write scaled value
      output.write();

      // wait for new input value (at the end, since we want to process the
      // initial values first)
      input.read();
    }
  }
};

template <typename InputType, typename OutputType = InputType,
          size_t NELEMS = 1>
struct Multiplier : public ApplicationModule {

  using ApplicationModule::ApplicationModule;

  Multiplier(EntityOwner *owner, const std::string &name,
             const std::string &factorName, const std::string &unitInput,
             const std::string &unitOutput, const std::string &description,
             const std::unordered_set<std::string> &tagsInput = {},
             const std::unordered_set<std::string> &tagsOutput = {},
             const std::unordered_set<std::string> &tagsFactor = {})
      : ApplicationModule(owner, name, "", true) {
    input.replace(ArrayPushInput<InputType>(this, name, unitInput, NELEMS,
                                            description, tagsInput));
    output.replace(ArrayOutput<OutputType>(this, name, unitOutput, NELEMS,
                                           description, tagsOutput));
    factor.replace(ScalarPushInput<double>(
        this, factorName, "(" + unitOutput + ")/(" + unitInput + ")",
        description, tagsFactor));
  }

  /** Note: This constructor is deprectated! */
  Multiplier(EntityOwner *owner, const std::string &name,
             const std::string &description)
      : ApplicationModule(owner, name, "", true),
        input(this, "input", "", NELEMS, description),
        factor(this, "factor", "", "Factor to scale the input value with"),
        output(this, "output", "", NELEMS, description) {}

  ArrayPushInput<InputType> input;
  ScalarPushInput<double> factor;
  ArrayOutput<OutputType> output;

  void mainLoop() {
    ReadAnyGroup group{input, factor};
    while (true) {

      // scale value (with rounding, if integral type)
      if (!std::numeric_limits<OutputType>::is_integer) {
        for (size_t i = 0; i < NELEMS; ++i)
          output[i] = input[i] * factor;
      } else {
        for (size_t i = 0; i < NELEMS; ++i)
          output[i] = std::round(input[i] * factor);
      }

      // write scaled value
      output.write();

      // wait for new input value (at the end, since we want to process the
      // initial values first)
      group.readAny();
    }
  }
};

template <typename InputType, typename OutputType = InputType,
          size_t NELEMS = 1>
struct Divider : public ApplicationModule {

  using ApplicationModule::ApplicationModule;
  Divider(EntityOwner *owner, const std::string &name,
          const std::string &description)
      : ApplicationModule(owner, name, ""),
        input(this, "input", "", NELEMS, description),
        divider(this, "divider", "", "Divider to scale the input value with"),
        output(this, "output", "", NELEMS, description) {
    setEliminateHierarchy();
  }

  ArrayPushInput<InputType> input;
  ScalarPushInput<double> divider;
  ArrayOutput<OutputType> output;

  void mainLoop() {
    ReadAnyGroup group{input, divider};
    while (true) {

      // scale value (with rounding, if integral type)
      if (!std::numeric_limits<OutputType>::is_integer) {
        for (size_t i = 0; i < NELEMS; ++i)
          output[i] = input[i] / divider;
      } else {
        for (size_t i = 0; i < NELEMS; ++i)
          output[i] = std::round(input[i] / divider);
      }

      // write scaled value
      output.write();

      // wait for new input value (at the end, since we want to process the
      // initial values first)
      group.readAny();
    }
  }
};

} // namespace ChimeraTK

#endif /* CHIMERATK_APPLICATION_CORE_MULTIPLIER_H */
