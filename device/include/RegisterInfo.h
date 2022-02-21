/*
 * RegisterInfo.h
 *
 *  Created on: Mar 1, 2016
 *      Author: Martin Hierholzer
 */

#pragma once

#include "ForwardDeclarations.h"
#include "RegisterInfoImpl.h"

#include <iostream>
#include <memory>

namespace ChimeraTK {

  class RegisterInfo {
   public:
    explicit RegisterInfo(std::unique_ptr<RegisterInfoImpl>&& impl);

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

    [[nodiscard]] RegisterInfoImpl& getImpl();

    [[nodiscard]] const RegisterInfoImpl& getImpl() const;

   protected:
    std::unique_ptr<RegisterInfoImpl> _impl;
  };

} /* namespace ChimeraTK */
