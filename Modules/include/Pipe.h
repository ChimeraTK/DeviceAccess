#ifndef CHIMERATK_APPLICATION_CORE_PIPE_H
#define CHIMERATK_APPLICATION_CORE_PIPE_H

#include <cmath>
#include <limits>

#include "ApplicationCore.h"

namespace ChimeraTK {

  /**
   *  Generic module to pipe through a scalar value without altering it.
   *  @todo Make it more efficient by removing this module entirely in the
   * ApplicationCore connection logic!
   */
  template<typename Type>
  struct ScalarPipe : public ApplicationModule {
    ScalarPipe(EntityOwner* owner, const std::string& name, const std::string& unit, const std::string& description,
        const std::unordered_set<std::string>& tagsInput = {}, const std::unordered_set<std::string>& tagsOutput = {})
    : ApplicationModule(owner, name, "", true) {
      input.replace(ScalarPushInput<Type>(this, name, unit, description, tagsInput));
      output.replace(ScalarOutput<Type>(this, name, unit, description, tagsOutput));
    }

    ScalarPipe(EntityOwner* owner, const std::string& inputName, const std::string& outputName, const std::string& unit,
        const std::string& description, const std::unordered_set<std::string>& tagsInput = {},
        const std::unordered_set<std::string>& tagsOutput = {})
    : ApplicationModule(owner, inputName, "", true) {
      input.replace(ScalarPushInput<Type>(this, inputName, unit, description, tagsInput));
      output.replace(ScalarOutput<Type>(this, outputName, unit, description, tagsOutput));
    }

    ScalarPipe() {}

    ScalarPushInput<Type> input;
    ScalarOutput<Type> output;

    void mainLoop() {
      while(true) {
        output = static_cast<Type>(input);
        output.write();
        input.read();
      }
    }
  };

  /**
   *  Generic module to pipe through a scalar value without altering it.
   *  @todo Make it more efficient by removing this module entirely in the
   * ApplicationCore connection logic!
   */
  template<typename Type>
  struct ArrayPipe : public ApplicationModule {
    ArrayPipe(EntityOwner* owner, const std::string& name, const std::string& unit, size_t nElements,
        const std::string& description, const std::unordered_set<std::string>& tagsInput = {},
        const std::unordered_set<std::string>& tagsOutput = {})
    : ApplicationModule(owner, name, description, true) {
      input.replace(ArrayPushInput<Type>(this, name, unit, nElements, description, tagsInput));
      output.replace(ArrayOutput<Type>(this, name, unit, nElements, description, tagsOutput));
    }

    ArrayPipe(EntityOwner* owner, const std::string& inputName, const std::string& outputName, const std::string& unit,
        size_t nElements, const std::string& description, const std::unordered_set<std::string>& tagsInput = {},
        const std::unordered_set<std::string>& tagsOutput = {})
    : ApplicationModule(owner, inputName, description, true) {
      input.replace(ArrayPushInput<Type>(this, inputName, unit, nElements, description, tagsInput));
      output.replace(ArrayOutput<Type>(this, outputName, unit, nElements, description, tagsOutput));
    }

    ArrayPipe() {}

    ArrayPushInput<Type> input;
    ArrayOutput<Type> output;

    void mainLoop() {
      std::vector<Type> temp(input.getNElements());
      while(true) {
        input.swap(temp);
        output.swap(temp);
        input.swap(temp);
        output.write();
        input.read();
      }
    }
  };

} // namespace ChimeraTK

#endif /* CHIMERATK_APPLICATION_CORE_PIPE_H */
