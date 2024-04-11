// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "BackendRegisterInfoBase.h"

#include <iostream>
#include <memory>

namespace ChimeraTK {

  class RegisterInfo {
   public:
    explicit RegisterInfo(std::unique_ptr<BackendRegisterInfoBase>&& impl);

    RegisterInfo(const RegisterInfo& other);
    RegisterInfo(RegisterInfo&& other) = default;

    RegisterInfo& operator=(const RegisterInfo& other);
    RegisterInfo& operator=(RegisterInfo&& other) = default;

    /** Return full path name of the register (including modules) */
    [[nodiscard]] RegisterPath getRegisterName() const;

    /** Return number of elements per channel */
    [[nodiscard]] unsigned int getNumberOfElements() const;

    /** Return number of channels in register */
    [[nodiscard]] unsigned int getNumberOfChannels() const;

    /** Return number of dimensions of this register */
    [[nodiscard]] unsigned int getNumberOfDimensions() const;

    /** Return desciption of the actual payload data for this register. See the
     * description of DataDescriptor for more information. */
    [[nodiscard]] const DataDescriptor& getDataDescriptor() const;

    /** Return whether the register is readable. */
    [[nodiscard]] bool isReadable() const;

    /** Return whether the register is writeable. */
    [[nodiscard]] bool isWriteable() const;

    /** Return all supported AccessModes for this register */
    [[nodiscard]] AccessModeFlags getSupportedAccessModes() const;

    /** Check whether the RegisterPath object is valid (i.e. contains an implementation object) */
    [[nodiscard]] bool isValid() const;

    /**
     * Return a reference to the implementation object. Only for advanced use, e.g. when backend-depending code shall
     * be written.
     */
    [[nodiscard]] BackendRegisterInfoBase& getImpl();

    /**
     * Return a const reference to the implementation object. Only for advanced use, e.g. when backend-depending code
     * shall be written.
     */
    [[nodiscard]] const BackendRegisterInfoBase& getImpl() const;

    /**
     *  Get the fully qualified async::SubDomain ID.
     *  If the register does not support wait_for_new_data it will be empty.
     *  Note: At the moment using async::Domain and async::SubDomain is not mandatory yet, so the ID might be empty even
     *  if the register supports wait_for_new_data.
     */
    [[nodiscard]] std::vector<size_t> getQualifiedAsyncId() const;

   protected:
    std::unique_ptr<BackendRegisterInfoBase> _impl;
  };

} /* namespace ChimeraTK */
