/*
 *
 */

#ifndef CHIMERATK_APPLICATION_CORE_MICRO_DAQ_H
#define CHIMERATK_APPLICATION_CORE_MICRO_DAQ_H

#include "ApplicationCore.h"
#include <ChimeraTK/SupportedUserTypes.h>

namespace ChimeraTK {

  namespace detail {
    template<typename TRIGGERTYPE>
    struct AccessorAttacher;
    template<typename TRIGGERTYPE>
    struct H5storage;
    template<typename TRIGGERTYPE>
    struct DataSpaceCreator;
    template<typename TRIGGERTYPE>
    struct DataWriter;
  } // namespace detail

  /**
   *  MicroDAQ module for logging data to HDF5 files. This can be usefull in
   * enviromenents where no sufficient logging of data is possible through the
   * control system. Any ChimeraTK::Module can act as a data source. Which
   * variables should be logged can be selected through EntityOwner::findTag().
   * 
   * ****************************************************************************************************************
   * IMPORTANT NOTE FOR ALL DEVELOPERS: This module is currently in the process of being moved into its own
   * repository and package. Please refrain from modifying the code here, as the changes might get lost!
   * ****************************************************************************************************************
   */
  template<typename TRIGGERTYPE = int32_t>
  class MicroDAQ : public ApplicationModule {
   public:
    /**
     *  Constructor. decimationFactor and decimationThreshold are configuration
     *  constants which determine how the data reduction is working. Arrays with a
     *  size bigger than decimationThreshold will be decimated by decimationFactor
     *  before writing to the HDF5 file.
     */
    MicroDAQ(EntityOwner* owner, const std::string& name, const std::string& description,
        uint32_t decimationFactor = 10, uint32_t decimationThreshold = 1000,
        HierarchyModifier hierarchyModifier = HierarchyModifier::none, const std::unordered_set<std::string>& tags = {},
        const std::string fileNamePrefix = "uDAQ/")
    : ApplicationModule(owner, name, description, hierarchyModifier, tags), decimationFactor_(decimationFactor),
      decimationThreshold_(decimationThreshold), fileNamePrefix_(fileNamePrefix) {}

    /** Deprecated constructor signature. Use the above signature instead. */
    [[deprecated]] MicroDAQ(EntityOwner* owner, const std::string& name, const std::string& description,
        uint32_t decimationFactor, uint32_t decimationThreshold, bool eliminateHierarchy,
        const std::unordered_set<std::string>& tags = {})
    : ApplicationModule(owner, name, description, eliminateHierarchy, tags), decimationFactor_(decimationFactor),
      decimationThreshold_(decimationThreshold), fileNamePrefix_("uDAQ/") {}

    /** Default constructor, creates a non-working module. Can be used for late
     * initialisation. */
    MicroDAQ() : decimationFactor_(0), decimationThreshold_(0) {}

    ScalarPushInput<TRIGGERTYPE> trigger{this, "trigger", "",
        "When written, the MicroDAQ write snapshot of all variables to the file", {"MicroDAQ.CONFIG"}};
    ScalarPushInput<int> enable{
        this, "enable", "", "DAQ is active when set to 0 and disabled when set to 0.", {"MicroDAQ.CONFIG"}};

    ScalarPollInput<uint32_t> nMaxFiles{this, "nMaxFiles", "",
        "Maximum number of files in the ring buffer (oldest file will be overwritten).", {"MicroDAQ.CONFIG"}};
    ScalarPollInput<uint32_t> nTriggersPerFile{
        this, "nTriggersPerFile", "", "Number of triggers stored in each file.", {"MicroDAQ.CONFIG"}};

    ScalarOutput<uint32_t> currentFile{
        this, "currentFile", "", "File number currently written to.", {"MicroDAQ.CONFIG"}};

    /** Add a Module as a source to this DAQ. */
    void addSource(const Module& source, const RegisterPath& namePrefix = "");

    /**
     * Overload that calls virtualiseFromCatalog.
     */
    void addSource(const DeviceModule& source, const RegisterPath& namePrefix = "");
   protected:
    void mainLoop() override;

    template<typename UserType>
    VariableNetworkNode getAccessor(const std::string& variableName);

    /** Map of VariableGroups required to build the hierarchies. The key it the
     * full path name. */
    std::map<std::string, VariableGroup> groupMap;

    /** boost::fusion::map of UserTypes to std::lists containing the
     * ArrayPushInput accessors. These accessors are dynamically created by the
     * AccessorAttacher. */
    template<typename UserType>
    using AccessorList = std::list<ArrayPushInput<UserType>>;
    TemplateUserTypeMap<AccessorList> accessorListMap;

    /** boost::fusion::map of UserTypes to std::lists containing the names of the
     * accessors. Technically there would be no need to use TemplateUserTypeMap
     * for this (as type does not depend on the UserType), but since these lists
     * must be filled consistently with the accessorListMap, the same construction
     * is used here. */
    template<typename UserType>
    using NameList = std::list<std::string>;
    TemplateUserTypeMap<NameList> nameListMap;

    /** Overall variable name list, used to detect name collisions */
    std::list<std::string> overallVariableList;

    /** Parameters for the data decimation */
    uint32_t decimationFactor_, decimationThreshold_;

    /** Prefix for the output files */
    std::string fileNamePrefix_;

    friend struct detail::AccessorAttacher<TRIGGERTYPE>;
    friend struct detail::H5storage<TRIGGERTYPE>;
    friend struct detail::DataSpaceCreator<TRIGGERTYPE>;
    friend struct detail::DataWriter<TRIGGERTYPE>;
  };

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(MicroDAQ);

} // namespace ChimeraTK

#endif /* CHIMERATK_APPLICATION_CORE_MICRO_DAQ_H */
