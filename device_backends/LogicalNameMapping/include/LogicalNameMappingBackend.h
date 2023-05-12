// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "BackendRegisterCatalogue.h"
#include "DeviceBackendImpl.h"
#include "LNMBackendRegisterInfo.h"
#include "LNMVariable.h"
#include <unordered_set>

#include <mutex>
#include <utility>

namespace ChimeraTK {

  /**
   * Backend to map logical register names onto real hardware registers. See \ref lmap for details.
   */
  class LogicalNameMappingBackend : public DeviceBackendImpl {
   public:
    explicit LogicalNameMappingBackend(std::string lmapFileName = "");

    void open() override;

    void close() override;

    bool isFunctional() const override;

    std::string readDeviceInfo() override { return std::string("Logical name mapping file: ") + _lmapFileName; }

    static boost::shared_ptr<DeviceBackend> createInstance(
        std::string address, std::map<std::string, std::string> parameters);

    RegisterCatalogue getRegisterCatalogue() const override;

    void setException() override;

    void activateAsyncRead() noexcept override;

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
    mutable BackendRegisterCatalogue<LNMBackendRegisterInfo> _catalogue_mutable;
    // mutable LNMRegisterCatalogue _catalogue_mutable;
    /** Flag whether the catalogue has already been filled with extra information
     * from the target backends */
    mutable bool catalogueCompleted{false};

    /** Struct holding shared accessors together with a mutex for thread safety. See sharedAccessorMap data member. */
    template<typename UserType>
    struct SharedAccessor {
      boost::weak_ptr<NDRegisterAccessor<UserType>> accessor;
      std::recursive_mutex mutex;

      // Must only be modified while holding mutex
      int useCount{0};
    };

    /** Map of target accessors which are potentially shared across our accessors. An example is the target accessors of
     *  LNMBackendBitAccessor. Multiple instances of LNMBackendBitAccessor referring to different bits of the same
     *  register share their target accessor. This sharing is governed by this map. */
    using AccessorKey = std::pair<DeviceBackend*, RegisterPath>;
    template<typename UserType>
    using SharedAccessorMap = std::map<AccessorKey, SharedAccessor<UserType>>;
    TemplateUserTypeMap<SharedAccessorMap> sharedAccessorMap;
    /// a mutex to be locked when sharedAccessorMap (the container) is changed
    std::mutex sharedAccessorMap_mutex;

    /** Map of variables and constants. This map contains the mpl tables with the actual values and a mutex for each of
     * them. It has to be mutable as the parse function must be const.
     */
    mutable std::map<std::string /*name*/, LNMVariable> _variables;

    template<typename T>
    friend class LNMBackendRegisterAccessor;

    template<typename T>
    friend class LNMBackendChannelAccessor;

    template<typename T>
    friend class LNMBackendBitAccessor;

    template<typename T>
    friend class LNMBackendVariableAccessor;

    /** Flag storing whether setException has been called. Cleared in open().
     */
    std::atomic<bool> _hasException{true};

    /// Flag storing whether asynchronous read has been activated.
    std::atomic<bool> _asyncReadActive{false};

    /** Obtain list of all target devices referenced in the catalogue */
    std::unordered_set<std::string> getTargetDevices() const;

   private:
    // A version number created when opening the backend. All variables will report this version number until they are
    // changed for the first time after opening the device.
    std::atomic<ChimeraTK::VersionNumber> _versionOnOpen{ChimeraTK::VersionNumber{nullptr}};

   public:
    ChimeraTK::VersionNumber getVersionOnOpen() const;
  };

} // namespace ChimeraTK
