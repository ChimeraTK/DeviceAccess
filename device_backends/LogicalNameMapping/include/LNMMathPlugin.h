// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "LNMAccessorPlugin.h"

#include <boost/make_shared.hpp>

#include <utility>

namespace ChimeraTK::LNMBackend {

  // forward declaration needed for MathPlugin
  struct MathPluginFormulaHelper;

  /** Math Plugin: Apply mathematical formula to register's data. The formula is parsed by the exprtk library. */
  class MathPlugin : public AccessorPlugin<MathPlugin> {
   public:
    MathPlugin(const LNMBackendRegisterInfo& info, size_t pluginIndex, std::map<std::string, std::string> parameters);

    void doRegisterInfoUpdate() override;
    DataType getTargetDataType(DataType) const override { return DataType::float64; }

    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target, const UndecoratedParams& accessorParams);

    void openHook(const boost::shared_ptr<LogicalNameMappingBackend>& backend) override;
    void postParsingHook(const boost::shared_ptr<const LogicalNameMappingBackend>& backend) override;
    void closeHook() override;
    void exceptionHook() override;

    /// if not yet existing, creates the instance and returns it
    /// if already existing, backend ptr may be empty, otherwise returns nullptr
    boost::shared_ptr<MathPluginFormulaHelper> getFormulaHelper(boost::shared_ptr<LogicalNameMappingBackend> backend);
    LNMBackendRegisterInfo* info() { return &_info; }

    bool _isWrite{false};

    std::map<std::string, std::string> _parameters;
    std::string _formula;              // extracted from _parameters
    bool _enablePushParameters{false}; // extracted from _parameters
    bool _hasPushParameter{false};     // only releant if _isWrite

    //  only used if _hasPushParameter == true
    // The _writeMutex has two functions:
    // - It protects resources which are shared by main accesor and parameter accessors
    // - It is held while an accessor is doing the preWrite/writeTransfer/postWrite sequence.
    //   If the other thread would be able to do a transfer bwetween the preWrite and the actual transfer this
    //   would lead to wrong results (although formally the code is thread safe)
    // Use a recursive mutex because it is allowed to call preWrite() multiple times before executing
    // the writeTransfer, and the mutex is accquired in preWrite() and release in only in postWrite().
    std::recursive_mutex _writeMutex;
    bool _mainValueWrittenAfterOpen{false};
    bool _allParametersWrittenAfterOpen{false};
    std::vector<double> _lastMainValue;
    ChimeraTK::DataValidity _lastMainValidity;

    bool _creatingFormulaHelper{false}; // a flag to prevent recursion

   private:
    // store weak pointer because plugin lifetime should not extend MathPluginFormulaHelper lifetime
    boost::weak_ptr<MathPluginFormulaHelper> _h;
  };

} // namespace ChimeraTK::LNMBackend
