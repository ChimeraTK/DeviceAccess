/*
 *  Generic module to take apart an array into individual values
 */

#ifndef CHIMERATK_APPLICATION_CORE_SPLIT_ARRAY_H
#define CHIMERATK_APPLICATION_CORE_SPLIT_ARRAY_H

#include "ApplicationCore.h"

namespace ChimeraTK {

  /** 
   * Split an array of the data type TYPE into nGroups with each nElemsPerGroup elements. The splitted array is an
   * output of this module and will be written everytime any of the inputs is updated. Each input is an array of the
   * length nElemsPerGroup and there are nGroups inputs. nElemsPerGroup defaults to 1, so the array is split into
   * its individual elements (and the inputs can be used as scalars).
   * 
   * The output array is called "output", while each input is called "input#", where # is the index of the input
   * counting from 0. When using the C++ interface, the inputs are stored in a std::vector and thus can be accessed
   * e.g. through "input[index]".
   * 
   * The output array has a size of nGroups*nElementsPerGroup.
   */
  template<typename TYPE, size_t nGroups, size_t nElemsPerGroup=1>
  struct WriteSplitArrayModule : public ApplicationModule {

      WriteSplitArrayModule(EntityOwner *owner, const std::string &name, const std::string &description);

      /** Vector of input arrays, each with a length of nElemsPerGroup. If nElemsPerGroup is 1 (default), the inputs
       *  can be used as scalars.
       *
       *  The input with the index "index" corresponds to the elements "index*nElemsPerGroup" to
       *  "(index+1)*nElemsPerGroup-1" of the output array. */
      std::vector<ArrayPushInput<TYPE>> input;
      
      /** Output array. Will be updated each time any input was changed with the corresponding data from the input. */
      ArrayOutput<TYPE> output{this, "output", "", nGroups*nElemsPerGroup, "Output array"};
      
      void mainLoop();

  };

  /*********************************************************************************************************************/

  /** 
   * Split an array of the data type TYPE into nGroups with each nElemsPerGroup elements. The splitted array is an
   * input to this module and ann outputs will be written to each time the input is updated. Each output is an array of
   * then length nElemsPerGroup and there are nGroups inputs. nElemsPerGroup defaults to 1, so the array is split into
   * its individual elements (and the outpus can be used as scalars).
   * 
   * The input array is called "input", while each output is called "output#", where # is the index of the output
   * counting from 0. When using the C++ interface, the outputs are stored in a std::vector and thus can be accessed
   * e.g. through "output[index]".
   * 
   * The input array has a size of nGroups*nElementsPerGroup.
   */
  template<typename TYPE, size_t nGroups, size_t nElemsPerGroup=1>
  struct ReadSplitArrayModule : public ApplicationModule {

      ReadSplitArrayModule(EntityOwner *owner, const std::string &name, const std::string &description);

      /** Vector of output arrays, each with a length of nElemsPerGroup. If nElemsPerGroup is 1 (default), the outputs
       *  can be used as scalars.
       *
       *  The output with the index "index" corresponds to the elements "index*nElemsPerGroup" to
       *  "(index+1)*nElemsPerGroup-1" of the input array. */
      std::vector<ArrayOutput<TYPE>> output;
      
      /** Input array. Each time this input is changed, all outputs will be updated with the corresponding data. */
      ArrayPushInput<TYPE> input{this, "input", "", nGroups*nElemsPerGroup, "Input array"};
      
      void mainLoop();

  };

  /*********************************************************************************************************************/
  /*********************************************************************************************************************/

  template<typename TYPE, size_t nGroups, size_t nElemsPerGroup>
  WriteSplitArrayModule<TYPE,nGroups,nElemsPerGroup>::WriteSplitArrayModule(EntityOwner *owner, const std::string &name,
                                                                            const std::string &description)
  : ApplicationModule(owner, name, description)
  {
    for(size_t i=0; i<nGroups; ++i) {
      std::string comment;
      if(nElemsPerGroup == 1) {
        comment = "The element "+std::to_string(i)+" of the output array";
      }
      else {
        comment = "The elements "+std::to_string(i*nElemsPerGroup)+" to "+
                  std::to_string((i+1)*nElemsPerGroup-1)+" of the output array";
      }
      input.push_back(ArrayPushInput<TYPE>(this, "input"+std::to_string(i), "", nElemsPerGroup, comment));
    }
  }

  /*********************************************************************************************************************/

  template<typename TYPE, size_t nGroups, size_t nElemsPerGroup>
  void WriteSplitArrayModule<TYPE,nGroups,nElemsPerGroup>::mainLoop() {
    while(true) {
      
      // write the array from the individual elements
      for(size_t i=0; i<nGroups; ++i) {
        for(size_t k=0; k<nElemsPerGroup; ++k) {
          output[i*nElemsPerGroup + k] = input[i][k];
        }
      }
      output.write();
      
      // wait for new input values (at the end, since we want to process the initial values first)
      readAny();
    }
  }

  /*********************************************************************************************************************/

  template<typename TYPE, size_t nGroups, size_t nElemsPerGroup>
  ReadSplitArrayModule<TYPE,nGroups,nElemsPerGroup>::ReadSplitArrayModule(EntityOwner *owner, const std::string &name,
                                                                          const std::string &description)
  : ApplicationModule(owner, name, description)
  {
    for(size_t i=0; i<nGroups; ++i) {
      std::string comment;
      if(nElemsPerGroup == 1) {
        comment = "The element "+std::to_string(i)+" of the input array";
      }
      else {
        comment = "The elements "+std::to_string(i*nElemsPerGroup)+" to "+
                  std::to_string((i+1)*nElemsPerGroup-1)+" of the input array";
      }
      output.push_back(ArrayOutput<TYPE>(this, "output"+std::to_string(i), "", nElemsPerGroup, comment));
    }
  }

  /*********************************************************************************************************************/
      
  template<typename TYPE, size_t nGroups, size_t nElemsPerGroup>
  void ReadSplitArrayModule<TYPE,nGroups,nElemsPerGroup>::mainLoop() {
    while(true) {
      
      // write the array from the individual elements
      for(size_t i=0; i<nGroups; ++i) {
        for(size_t k=0; k<nElemsPerGroup; ++k) {
          input[i][k] = output[i*nElemsPerGroup + k];
        }
      }
      writeAll();
      
      // wait for new input values (at the end, since we want to process the initial values first)
      input.read();
    }
  }

} // namespace ChimeraTK

#endif /* CHIMERATK_APPLICATION_CORE_SPLIT_ARRAY_H */
