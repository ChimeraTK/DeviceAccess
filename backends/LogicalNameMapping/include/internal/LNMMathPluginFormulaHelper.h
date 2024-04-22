// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

// note this header is internal (i.e. should not be installed as part of DeviceAccess API) and is the only place
// where exprtk is included.
// So this header must only be included from .cc files, in order to keep exprtk hidden

#include "LNMAccessorPlugin.h"

#include <boost/make_shared.hpp>

#include <exprtk.hpp>
#include <utility>

namespace ChimeraTK::LNMBackend {

  class MathPlugin;

  class MathPluginFormulaHelper {
   public:
    MathPluginFormulaHelper(MathPlugin* p, const boost::shared_ptr<LogicalNameMappingBackend>& backend);

    void compileFormula(const std::string& formula,
        const std::map<std::string, boost::shared_ptr<ChimeraTK::NDRegisterAccessor<double>>>& parameters,
        size_t nElements);

    template<typename T>
    void computeResult(std::vector<double>& x, std::vector<T>& resultBuffer);

    // This function updates all parameter accessors and return their worst validity
    [[nodiscard]] ChimeraTK::DataValidity updateParameters();

    // This function updates result in target based on latest values of parameter accessors and lastMainValue
    void updateResult(ChimeraTK::VersionNumber versionNumber);

    // Checks that all parameters have been written since opening the device.
    // Returns false as long as at least one parameter is still on the backend's _versionOnOpen.
    // Only call this function when holding the _writeMutex. It updates the _allParametersWrittenAfterOpen
    // variable which is protected by that mutex.
    bool checkAllParametersWritten();

    // set the exception backend for all parameter accessors
    void setExceptionBackend(boost::shared_ptr<DeviceBackend> exceptionBackend);

    // return the LNM backend
    boost::shared_ptr<LogicalNameMappingBackend> getBackend();

   protected:
    std::string varName;
    exprtk::expression<double> expression;
    exprtk::symbol_table<double> symbols;
    exprtk::rtl::vecops::package<double> vecOpsPkg;
    std::unique_ptr<exprtk::vector_view<double>> valueView;
    std::map<boost::shared_ptr<NDRegisterAccessor<double>>, std::unique_ptr<exprtk::vector_view<double>>> params;

    boost::shared_ptr<LogicalNameMappingBackend> _backend;
    boost::shared_ptr<NDRegisterAccessor<double>> _target;

    // We assume plugin lives at least as long as MathPluginFormulaHelper
    MathPlugin* _mp;

    std::map<std::string, boost::shared_ptr<NDRegisterAccessor<double>>> _accessorMap;
  };

} // namespace ChimeraTK::LNMBackend
