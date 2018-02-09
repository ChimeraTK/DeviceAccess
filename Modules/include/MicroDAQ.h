/*
 *
 */

#ifndef CHIMERATK_APPLICATION_CORE_MICRO_DAQ_H
#define CHIMERATK_APPLICATION_CORE_MICRO_DAQ_H

#include "ApplicationCore.h"
#include <mtca4u/SupportedUserTypes.h>

namespace ChimeraTK {

  struct MicroDAQ : public ApplicationModule {
      using ApplicationModule::ApplicationModule;

      ScalarPushInput<int> trigger{this, "trigger", "", "When written, the MicroDAQ write snapshot of all variables "
                "to the file", {"MicroDAQ.CONFIG"}};
      ScalarPollInput<int> enable{this, "enable", "", "DAQ is active when set to 0 and disabled when set to 0.",
                {"MicroDAQ.CONFIG"}};

      ScalarPollInput<uint32_t> nMaxFiles{this, "nMaxFiles", "", "Maximum number of files in the ring buffer "
                "(oldest file will be overwritten).", {"MicroDAQ.CONFIG"}};
      ScalarPollInput<uint32_t> nTriggersPerFile{this, "nTriggersPerFile", "",
                "Number of triggers stored in each file.", {"MicroDAQ.CONFIG"}};

      ScalarOutput<uint32_t> currentFile{this, "currentFile", "", "File number currently written to.",
                {"MicroDAQ.CONFIG"}};

      void mainLoop() override;

      /** Add a Module as a source to this DAQ. */
      void addSource(const Module &source, const std::string &namePrefix="");

      template<typename UserType>
      VariableNetworkNode getAccessor(const std::string &variableName);

      /** boost::fusion::map of UserTypes to std::lists containing the ArrayPollInput accessors. These accessors are
      *  dynamically created by the AccessorAttacher. */
      template<typename UserType>
      using AccessorList = std::list<ArrayPollInput<UserType>>;
      TemplateUserTypeMap<AccessorList> accessorListMap;

      /** boost::fusion::map of UserTypes to std::lists containing the names of the accessors. Technically there would
      *  be no need to use TemplateUserTypeMap for this (as type does not depend on the UserType), but since these
      *  lists must be filled consistently with the accessorListMap, the same construction is used here. */
      template<typename UserType>
      using NameList = std::list<std::string>;
      TemplateUserTypeMap<NameList> nameListMap;

      /** Overall variable name list, used to detect name collisions */
      std::list<std::string> overallVariableList;

  };

} // namespace ChimeraTK

#endif /* CHIMERATK_APPLICATION_CORE_MICRO_DAQ_H */
