#ifndef CHIMERATK_APPLICATION_CORE_PIPE_H
#define CHIMERATK_APPLICATION_CORE_PIPE_H

#include <limits>
#include <cmath>

#include "ApplicationCore.h"

namespace ChimeraTK {

  /**
  *  Generic module to pipe through a scalar value without altering it.
  *  @todo Make it more efficient by removing this module entirely in the ApplicationCore connection logic!
  */
  template<typename Type>
  struct ScalarPipe : public ApplicationModule {
      
      ScalarPipe(Module *owner, const std::string &name, const std::string &unit, const std::string &description,
                const std::unordered_set<std::string> &tagsInput={}, const std::unordered_set<std::string> &tagsOutput={})
      : ApplicationModule(owner, name, description, true)
      {
        input.replace(ScalarPushInput<Type>(owner, name, unit, description, tagsInput));
        output.replace(ScalarOutput<Type>(owner, name, unit, description, tagsOutput));
      }
      
      ScalarPipe(Module *owner, const std::string &inputName, const std::string &outputName, const std::string &unit,
                const std::string &description,
                const std::unordered_set<std::string> &tagsInput={}, const std::unordered_set<std::string> &tagsOutput={})
      : ApplicationModule(owner, inputName, description, true)
      {
        input.replace(ScalarPushInput<Type>(owner, inputName, unit, description, tagsInput));
        output.replace(ScalarOutput<Type>(owner, outputName, unit, description, tagsOutput));
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
  *  @todo Make it more efficient by removing this module entirely in the ApplicationCore connection logic!
  */
  template<typename Type>
  struct ArrayPipe : public ApplicationModule {
      
      ArrayPipe(Module *owner, const std::string &name, const std::string &unit, size_t nElements, const std::string &description,
                const std::unordered_set<std::string> &tagsInput={}, const std::unordered_set<std::string> &tagsOutput={})
      : ApplicationModule(owner, name, description, true)
      {
        input.replace(ArrayPushInput<Type>(owner, name, unit, nElements, description, tagsInput));
        output.replace(ArrayOutput<Type>(owner, name, unit, nElements, description, tagsOutput));
      }
      
      ArrayPipe(Module *owner, const std::string &inputName, const std::string &outputName, const std::string &unit,
                size_t nElements, const std::string &description,
                const std::unordered_set<std::string> &tagsInput={}, const std::unordered_set<std::string> &tagsOutput={})
      : ApplicationModule(owner, inputName, description, true)
      {
        input.replace(ArrayPushInput<Type>(owner, inputName, unit, nElements, description, tagsInput));
        output.replace(ArrayOutput<Type>(owner, outputName, unit, nElements, description, tagsOutput));
      }

      ArrayPipe() {}

      ArrayPushInput<Type> input;
      ArrayOutput<Type> output;
      
      void mainLoop() {
        std::vector<int> temp(input.getNElements());
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
