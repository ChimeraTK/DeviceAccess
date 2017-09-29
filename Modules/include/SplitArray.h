/*
 *  Generic module to take apart an array into individual values
 */

#ifndef CHIMERATK_APPLICATION_CORE_SPLIT_ARRAY_H
#define CHIMERATK_APPLICATION_CORE_SPLIT_ARRAY_H

#include "ApplicationCore.h"

namespace ChimeraTK {

  template<typename TYPE, size_t NELEMS>
  struct WriteSplitArrayModule : public ApplicationModule {
      using ApplicationModule::ApplicationModule;

      /// individual inputs for each element
      struct Input : public VariableGroup {
        Input() {}
        Input(EntityOwner *owner, const std::string &name, const std::string &description)
        : VariableGroup(owner, name, description)
        {
          for(size_t i=0; i<NELEMS; ++i) {
            elem[i].replace(ScalarPushInput<TYPE>(this, "elem"+std::to_string(i), "", "The element "+std::to_string(i)+" of the array"));
          }
        }
        
        ScalarPushInput<TYPE> elem[NELEMS];
      };
      Input input{this, "input", ""};

      ArrayOutput<TYPE> output{this, "output", "", NELEMS, "Output array"};
      
      void mainLoop() {
        while(true) {
          
          // write the array from the individual elements
          for(size_t i=0; i<NELEMS; ++i) {
            output[i] = input.elem[i];
          }
          output.write();
          
          // wait for new input values (at the end, since we want to process the initial values first)
          input.readAny();
        }
      }

  };

  /*********************************************************************************************************************/

  template<typename TYPE, size_t NELEMS>
  struct ReadSplitArrayModule : public ApplicationModule {
      using ApplicationModule::ApplicationModule;

      /// individual outputs for each element
      struct Output : public VariableGroup {
        Output() {}
        Output(EntityOwner *owner, const std::string &name, const std::string &description)
        : VariableGroup(owner, name, description)
        {
          for(size_t i=0; i<NELEMS; ++i) {
            elem[i].replace(ScalarOutput<TYPE>(this, "elem"+std::to_string(i), "", "The element "+std::to_string(i)+" of the array"));
          }
        }
        
        ScalarOutput<TYPE> elem[NELEMS];
      };
      Output output{this, "output", ""};

      ArrayPushInput<TYPE> input{this, "input", "", NELEMS, "Input array"};
      
      void mainLoop() {
        while(true) {
          
          // take apart the aray
          for(size_t i=0; i<NELEMS; ++i) {
            output.elem[i] = input[i];
            output.elem[i].write();                        /// @todo TODO better make a writeAll() for VariableGroups
          }
          
          // wait for new input values (at the end, since we want to process the initial values first)
          input.read();
        }
      }

  };

} // namespace ChimeraTK

#endif /* CHIMERATK_APPLICATION_CORE_SPLIT_ARRAY_H */
