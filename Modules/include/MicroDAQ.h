/*
 *
 */

#ifndef CHIMERATK_APPLICATION_CORE_MICRO_DAQ_H
#define CHIMERATK_APPLICATION_CORE_MICRO_DAQ_H

#include "ApplicationCore.h"
#include <ChimeraTK/SupportedUserTypes.h>

namespace ChimeraTK {

  namespace detail {
    struct AccessorAttacher;
    struct H5storage;
    struct DataSpaceCreator;
    struct DataWriter;
  }

  /**
   *  MicroDAQ module for logging data to HDF5 files. This can be usefull in enviromenents where no sufficient logging
   *  of data is possible through the control system. Any ChimeraTK::Module can act as a data source. Which variables
   *  should be logged can be selected through EntityOwner::findTag().
   */
  struct MicroDAQ : public ApplicationModule {

      /**
       *  Constructor. decimationFactor and decimationThreshold are configuration constants which determine how the
       *  data reduction is working. Arrays with a size bigger than decimationThreshold will be decimated by
       *  decimationFactor before writing to the HDF5 file.
       */
      MicroDAQ(EntityOwner *owner, const std::string &name, const std::string &description, uint32_t decimationFactor=10,
               uint32_t decimationThreshold=1000, bool eliminateHierarchy=false,
               const std::unordered_set<std::string> &tags={})
      : ApplicationModule(owner, name, description, eliminateHierarchy, tags),
        decimationFactor_(decimationFactor), decimationThreshold_(decimationThreshold) {}

      /** Default constructor, creates a non-working module. Can be used for late initialisation. */
      MicroDAQ() : decimationFactor_(0), decimationThreshold_(0) {}

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

      /** Add a Module as a source to this DAQ. */
      void addSource(const Module &source, const std::string &namePrefix="");

    protected:

      void mainLoop() override;

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

      /** Parameters for the data decimation */
      uint32_t decimationFactor_, decimationThreshold_;

      friend struct detail::AccessorAttacher;
      friend struct detail::H5storage;
      friend struct detail::DataSpaceCreator;
      friend struct detail::DataWriter;

  };

} // namespace ChimeraTK

#endif /* CHIMERATK_APPLICATION_CORE_MICRO_DAQ_H */
