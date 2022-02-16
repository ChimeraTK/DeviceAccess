#ifndef REGISTERINFOIMPL_H
#define REGISTERINFOIMPL_H

#include "DataDescriptor.h"
#include "AccessMode.h"
#include "RegisterPath.h"

namespace ChimeraTK {

/*******************************************************************************************************************/

  /** DeviceBackend-independent register description. */
  class RegisterInfoImpl {
   public:

    /** Virtual destructor */
    virtual ~RegisterInfoImpl() {}

    /** Return full path name of the register (including modules) */
    virtual RegisterPath getRegisterName() const = 0;

    /** Return number of elements per channel */
    virtual unsigned int getNumberOfElements() const = 0;

    /** Return number of channels in register */
    virtual unsigned int getNumberOfChannels() const = 0;

    /** Return number of dimensions of this register */
    virtual unsigned int getNumberOfDimensions() const = 0;

    /** Return desciption of the actual payload data for this register. See the
     * description of DataDescriptor for more information. */
    virtual const DataDescriptor& getDataDescriptor() const = 0;

    /** Return whether the register is readable. */
    virtual bool isReadable() const = 0;

    /** Return whether the register is writeable. */
    virtual bool isWriteable() const = 0;

    /** Return all supported AccessModes for this register */
    virtual AccessModeFlags getSupportedAccessModes() const = 0;
  };

}

#endif // REGISTERINFOIMPL_H
