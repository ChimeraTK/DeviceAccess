#pragma once

#include "AccessMode.h"
#include "DataDescriptor.h"
#include "RegisterPath.h"

#include <memory>

namespace ChimeraTK {

  /*******************************************************************************************************************/

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
    [[nodiscard]] virtual std::unique_ptr<BackendRegisterInfoBase> clone() const = 0;
  };

  /*******************************************************************************************************************/

  inline unsigned int BackendRegisterInfoBase::getNumberOfDimensions() const {
    if(getNumberOfChannels() > 1) return 2;
    if(getNumberOfElements() > 1) return 1;
    return 0;
  }

} // namespace ChimeraTK
