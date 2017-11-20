/*
 * FanOut.h
 *
 *  Created on: Jun 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_FAN_OUT_H
#define CHIMERATK_FAN_OUT_H

#include <mtca4u/NDRegisterAccessor.h>

#include "ApplicationException.h"

namespace ChimeraTK {

  /** Base class for several implementations which distribute values from one feeder to multiple consumers */
  template<typename UserType>
  class FanOut {

    public:

      FanOut(boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> feedingImpl)
      : impl(feedingImpl)
      {}

      virtual ~FanOut() {}

      /** Add a slave to the FanOut. Only sending end-points of a consuming node may be added. */
      virtual void addSlave(boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> slave) {
        if(!slave->isWriteable()) {
          throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(
              "FanOut::addSlave() has been called with a receiving implementation!");
        }
        // check if array shape is compatible, unless the receiver is a trigger node, so no data is expected
        if( slave->getNumberOfSamples() != 0 &&
            ( slave->getNumberOfChannels() != impl->getNumberOfChannels() ||
              slave->getNumberOfSamples() != impl->getNumberOfSamples()      ) ) {
          std::string what = "FanOut::addSlave(): Trying to add a slave '";
          what += slave->getName();
          what += "' with incompatible array shape! Name of master: ";
          what += impl->getName();
          throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(what.c_str());
        }
        slaves.push_back(slave);
      }

    protected:

      boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> impl;

      std::list<boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>>> slaves;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_FAN_OUT_H */
