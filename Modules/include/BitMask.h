/*
 *  Generic module to take apart a bit field into individual values
 */

#ifndef CHIMERATK_APPLICATION_CORE_BIT_MASK_H
#define CHIMERATK_APPLICATION_CORE_BIT_MASK_H

#include "ApplicationCore.h"

namespace ChimeraTK {

  template<size_t NBITS>
  struct WriteBitMask : public ApplicationModule {
      using ApplicationModule::ApplicationModule;

      /// individual inputs for each bit
      struct Input : public VariableGroup {
        Input() {}
        Input(EntityOwner *owner, const std::string &name, const std::string &description)
        : VariableGroup(owner, name, description)
        {
          setEliminateHierarchy();
          for(size_t i=0; i<NBITS; ++i) {
            bit[i].replace(ScalarPushInput<int>(this, "bit"+std::to_string(i), "", "The bit "+std::to_string(i)+" of the bit mask"));
          }
        }

        ScalarPushInput<int> bit[NBITS];
      };
      Input input{this, "input", "The input bits"};

      ScalarOutput<int32_t> bitmask{this, "bitmask", "", "Output bit mask."};

      void mainLoop() {
        while(true) {

          // create bit mask
          bitmask = 0;
          for(size_t i=0; i<NBITS; ++i) {
            if(input.bit[i] != 0) {
              bitmask |= 1 << i;
            }
          }
          bitmask.write();

          // wait for new input values (at the end, since we want to process the initial values first)
          input.readAny();
        }
      }

  };

  /*********************************************************************************************************************/

  template<size_t NBITS>
  struct ReadBitMask : public ApplicationModule {
      ReadBitMask(EntityOwner *owner, const std::string &name, const std::string &description,
                  bool eliminateHierarchy=false, const std::unordered_set<std::string> &tags={})
      : ApplicationModule(owner, name, description, eliminateHierarchy, tags) {}

      ReadBitMask() {}

      ReadBitMask( EntityOwner *owner, const std::string &inputName, const std::string &inputDescription,
                   const std::unordered_set<std::string> &inputTags, const std::array<std::string, NBITS> &outputNames,
                   const std::array<std::string, NBITS> &outputDescriptions, const std::unordered_set<std::string> &outputTags )
      : ApplicationModule(owner, inputName, inputDescription, true)
      {
        bitmask.setMetaData(inputName, "", inputDescription, inputTags);
        output.setEliminateHierarchy();
        for(size_t i=0; i<NBITS; ++i) {
          output.bit[i].setMetaData(outputNames[i], "", outputDescriptions[i], outputTags);
        }
      }

      /// individual outputs for each bit
      struct Output : public VariableGroup {
        Output() {}
        Output(EntityOwner *owner, const std::string &name, const std::string &description)
        : VariableGroup(owner, name, description)
        {
          setEliminateHierarchy();
          for(size_t i=0; i<NBITS; ++i) {
            bit[i].replace(ScalarOutput<int>(this, "bit"+std::to_string(i), "", "The bit "+std::to_string(i)+" of the bit mask"));
          }
        }

        ScalarOutput<int> bit[NBITS];
      };
      Output output{this, "output", "The extracted output bits"};

      ScalarPushInput<int32_t> bitmask{this, "bitmask", "", "Input bit mask."};

      void mainLoop() {
        while(true) {

          // decode bit mask
          for(size_t i=0; i<NBITS; ++i) {
            output.bit[i] = (bitmask & (1 << i)) != 0;
            output.bit[i].write();                        /// @todo TODO better make a writeAll() for VariableGroups
          }

          // wait for new input values (at the end, since we want to process the initial values first)
          bitmask.read();
        }
      }

  };

} // namespace ChimeraTK

#endif /* CHIMERATK_APPLICATION_CORE_BIT_MASK_H */
