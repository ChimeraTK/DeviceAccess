// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AccessMode.h"
#include "DataDescriptor.h"
#include "RegisterPath.h"

#include <memory>

namespace ChimeraTK {

  /********************************************************************************************************************/

  /** DeviceBackend-independent register description. */
  class BackendRegisterInfoBase {
   public:
    /** Virtual destructor */
    virtual ~BackendRegisterInfoBase() = default;

    /** Return full path name of the register (including modules) */
    [[nodiscard]] virtual RegisterPath getRegisterName() const = 0;

    /** Return number of elements per channel */
    [[nodiscard]] virtual unsigned int getNumberOfElements() const = 0;

    /** Return number of channels in register */
    [[nodiscard]] virtual unsigned int getNumberOfChannels() const = 0;

    /** Return number of dimensions of this register */
    [[nodiscard]] unsigned int getNumberOfDimensions() const;

    /** Return description of the actual payload data for this register. See the
     * description of DataDescriptor for more information. */
    [[nodiscard]] virtual const DataDescriptor& getDataDescriptor() const = 0;

    /** Return whether the register is readable. */
    [[nodiscard]] virtual bool isReadable() const = 0;

    /** Return whether the register is writeable. */
    [[nodiscard]] virtual bool isWriteable() const = 0;

    /** Return all supported AccessModes for this register */
    [[nodiscard]] virtual AccessModeFlags getSupportedAccessModes() const = 0;

    /** Create copy of the object */
    [[nodiscard]] virtual std::unique_ptr<BackendRegisterInfoBase> clone() const = 0;

    /**
     *  Return the fully qualified async::SubDomain ID.
     *  The default implementation returns an empty vector.
     */
    [[nodiscard]] virtual std::vector<size_t> getQualifiedAsyncId() const { return {}; }

    /**
     * Get the list of tags associated with this register.
     *
     * The default implementation returns an empty list.
     */
    [[nodiscard]] virtual std::set<std::string> getTags() const { return {}; };

    /**
     * Returns whether the register is "hidden", meaning it won't be listed when iterating the catalogue.
     *
     * Hidden registers can be explicitly iterated, but the ordinary iterators will not show them.
     */
    [[nodiscard]] virtual bool isHidden() const { return false; }

    /** Default comparison so inheriting classes can define a default comparison. */
    bool operator==(const BackendRegisterInfoBase&) const = default;
  };

  /********************************************************************************************************************/

  inline unsigned int BackendRegisterInfoBase::getNumberOfDimensions() const {
    if(getNumberOfChannels() > 1) return 2;
    if(getNumberOfElements() > 1) return 1;
    return 0;
  }

} // namespace ChimeraTK
