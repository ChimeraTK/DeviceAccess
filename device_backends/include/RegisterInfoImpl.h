#pragma once

#include "DataDescriptor.h"
#include "AccessMode.h"
#include "RegisterPath.h"

#include <memory>

namespace ChimeraTK {

/*******************************************************************************************************************/

  /** DeviceBackend-independent register description. */
  class RegisterInfoImpl {
   public:

    /** Virtual destructor */
    virtual ~RegisterInfoImpl() = default;

    /** Return full path name of the register (including modules) */
    [[nodiscard]] virtual RegisterPath getRegisterName() const = 0;

    /** Return number of elements per channel */
    [[nodiscard]] virtual unsigned int getNumberOfElements() const = 0;

    /** Return number of channels in register */
    [[nodiscard]] virtual unsigned int getNumberOfChannels() const = 0;

    /** Return number of dimensions of this register */
    [[nodiscard]] virtual unsigned int getNumberOfDimensions() const = 0;

    /** Return desciption of the actual payload data for this register. See the
     * description of DataDescriptor for more information. */
    [[nodiscard]] virtual const DataDescriptor& getDataDescriptor() const = 0;

    /** Return whether the register is readable. */
    [[nodiscard]] virtual bool isReadable() const = 0;

    /** Return whether the register is writeable. */
    [[nodiscard]] virtual bool isWriteable() const = 0;

    /** Return all supported AccessModes for this register */
    [[nodiscard]] virtual AccessModeFlags getSupportedAccessModes() const = 0;

    /** Create copy of the object */
    [[nodiscard]] virtual std::unique_ptr<RegisterInfoImpl> clone() const = 0;
  };

} // namespace ChimeraTK
