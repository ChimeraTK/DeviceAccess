/*
 * ScalarRegisterAccessor.h
 *
 *  Created on: Mar 23, 2016
 *      Author: Martin Hierholzer <martin.hierholzer@desy.de>
 */

#pragma once

#include "NDRegisterAccessorAbstractor.h"

namespace ChimeraTK {

  // We cannot use a typedef because the constructor of NDRegisterAccessorAbstractor is intentionally
  // protected.
  class VoidRegisterAccessor : public NDRegisterAccessorAbstractor<ChimeraTK::Void> {
   public:
    VoidRegisterAccessor(boost::shared_ptr<NDRegisterAccessor<Void>> impl);
    bool isReadOnly() const;
    bool isReadable() const;
    void read();
    bool readNonBlocking();
    bool readLatest();
  };

} // namespace ChimeraTK
