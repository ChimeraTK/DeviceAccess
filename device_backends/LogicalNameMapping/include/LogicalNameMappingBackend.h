/*
 * LogicalNameMappingBackend.h
 *
 *  Created on: Feb 8, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_LOGICAL_NAME_MAPPING_BACKEND_H
#define CHIMERA_TK_LOGICAL_NAME_MAPPING_BACKEND_H

#include "DeviceBackendImpl.h"
#include "LNMBackendRegisterInfo.h"

namespace ChimeraTK {

  /**
   *  Backend to map logical register names onto real hardware registers. It reads
   * the logical name map from an xml file and will open internally additional
   * Devices as they are reference in that file.
   *
   *  The map file should be specified by the "map" parameter in the CDD, i.e.:
   *    (logicalNameMap?map=path/to/mapfile.xlmap)
   *
   *  Additional parameters can be passed through the CDD, which are available
   * inside the xlmap file via &gt;par&lt;&gt;/par&lt; tags, e.g. when using the
   * following CDD:
   *
   *     (logicalNameMap?map=path/to/mapfile.xlmap&myParam=HelloWorld)
   *
   *  the tag:
   *
   *     &gt;par&lt;myParam&gt;/par&lt;
   *
   *  inside the xlmap file
   */
  class LogicalNameMappingBackend : public DeviceBackendImpl {
   public:
    LogicalNameMappingBackend(std::string lmapFileName = "");

    ~LogicalNameMappingBackend() override {}

    void open() override;

    void close() override;

    std::string readDeviceInfo() override { return std::string("Logical name mapping file: ") + _lmapFileName; }

    static boost::shared_ptr<DeviceBackend> createInstance(std::string address,
        std::map<std::string, std::string>
            parameters);

    const RegisterCatalogue& getRegisterCatalogue() const override;

   protected:
    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(const RegisterPath& registerPathName,
        size_t numberOfWords,
        size_t wordOffsetInRegister,
        AccessModeFlags flags);
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(LogicalNameMappingBackend, getRegisterAccessor_impl, 4);

    /// parse the logical map file, if not yet done
    void parse();

    /// flag if already parsed
    bool hasParsed;

    /// name of the logical map file
    std::string _lmapFileName;

    /// map of target devices
    std::map<std::string, boost::shared_ptr<DeviceBackend>> _devices;

    /// map of parameters passed through the CDD
    std::map<std::string, std::string> _parameters;

    /** We need to make the catalogue mutable, since we fill it within
     * getRegisterCatalogue() */
    mutable RegisterCatalogue _catalogue_mutable;

    /** Flag whether the catalogue has already been filled with extra information
     * from the target backends */
    mutable bool catalogueCompleted{false};

    template<typename T>
    friend class LNMBackendRegisterAccessor;

    template<typename T>
    friend class LNMBackendChannelAccessor;

    template<typename T>
    friend class LNMBackendBitAccessor;

    template<typename T>
    friend class LNMBackendVariableAccessor;
  };

} // namespace ChimeraTK

#endif /* CHIMERA_TK_LOGICAL_NAME_MAPPING_BACKEND_H */
