/*
 * RegisterInfo.h
 *
 *  Created on: Mar 1, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_REGISTER_INFO_H
#define CHIMERA_TK_REGISTER_INFO_H

#include "ForwardDeclarations.h"
#include "RegisterInfoImpl.h"
#include <boost/shared_ptr.hpp>

#include <iostream>

namespace ChimeraTK {

  class RegisterInfo {
   public:
    RegisterInfo(boost::shared_ptr<RegisterInfoImpl> impl);

    /** Return full path name of the register (including modules) */
    RegisterPath getRegisterName() const;

    /** Return number of elements per channel */
    unsigned int getNumberOfElements() const;

    /** Return number of channels in register */
    unsigned int getNumberOfChannels() const;

    /** Return number of dimensions of this register */
    unsigned int getNumberOfDimensions() const;

    /** Return desciption of the actual payload data for this register. See the
     * description of DataDescriptor for more information. */
    const DataDescriptor& getDataDescriptor() const;

    /** Return whether the register is readable. */
    bool isReadable() const;

    /** Return whether the register is writeable. */
    bool isWriteable() const;

    /** Return all supported AccessModes for this register */
    AccessModeFlags getSupportedAccessModes() const;

    boost::shared_ptr<RegisterInfoImpl> getImpl(); // find better name

   protected:
    boost::shared_ptr<RegisterInfoImpl> impl;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERA_TK_REGISTER_INFO_H */
