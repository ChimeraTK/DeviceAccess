/*
 * LogicalNameMappingBackend.h
 *
 *  Created on: Feb 8, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_LOGICAL_NAME_MAPPING_BACKEND_H
#define CHIMERA_TK_LOGICAL_NAME_MAPPING_BACKEND_H

#include <mutex>

#include "DeviceBackendImpl.h"
#include "LNMBackendRegisterInfo.h"

namespace ChimeraTK {

  /**
   * Backend to map logical register names onto real hardware registers. See \ref lmap for details.
   */
  class LogicalNameMappingBackend : public DeviceBackendImpl {
   public:
    LogicalNameMappingBackend(std::string lmapFileName = "");

    ~LogicalNameMappingBackend() override {}

    void open() override;

    void close() override;

    std::string readDeviceInfo() override { return std::string("Logical name mapping file: ") + _lmapFileName; }

    static boost::shared_ptr<DeviceBackend> createInstance(
        std::string address, std::map<std::string, std::string> parameters);

    const RegisterCatalogue& getRegisterCatalogue() const override;

    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(const RegisterPath& registerPathName,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags, size_t omitPlugins = 0);

    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_internal(
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

    /// parse the logical map file, if not yet done
    void parse() const;

    /// flag if already parsed
    mutable bool hasParsed;

    /// name of the logical map file
    std::string _lmapFileName;

    /// map of target devices
    mutable std::map<std::string, boost::shared_ptr<DeviceBackend>> _devices;

    /// map of parameters passed through the CDD
    std::map<std::string, std::string> _parameters;

    /** We need to make the catalogue mutable, since we fill it within
     * getRegisterCatalogue() */
    mutable RegisterCatalogue _catalogue_mutable;

    /** Flag whether the catalogue has already been filled with extra information
     * from the target backends */
    mutable bool catalogueCompleted{false};

    /** Struct holding shared accessors together with a mutex for thread safety. See sharedAccessorMap data member. */
    template<typename UserType>
    struct SharedAccessor {
      boost::shared_ptr<NDRegisterAccessor<UserType>> accessor;
      std::mutex mutex;
    };

    /** Map of target accessors which are potentially shared across our accessors. An example is the target accessors of
     *  LNMBackendBitAccessor. Multiple instances of LNMBackendBitAccessor referring to different bits of the same
     *  register share their target accessor. This sharing is governed by this map. */
    template<typename UserType>
    using SharedAccessorMap = std::map<std::string, SharedAccessor<UserType>>;
    TemplateUserTypeMap<SharedAccessorMap> sharedAccessorMap;

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
