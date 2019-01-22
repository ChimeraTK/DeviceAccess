/*
 * FanOut.h
 *
 *  Created on: Jun 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_FAN_OUT_H
#define CHIMERATK_FAN_OUT_H

#include <ChimeraTK/NDRegisterAccessor.h>

namespace ChimeraTK {

  /** Base class for several implementations which distribute values from one feeder to multiple consumers */
  template<typename UserType>
  class FanOut {

    public:

      FanOut(boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> feedingImpl)
      : impl(feedingImpl)
      {}

      virtual ~FanOut() {}

      /** Add a slave to the FanOut. Only sending end-points of a consuming node may be added. */
      virtual void addSlave(boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> slave) {
        if(!slave->isWriteable()) {
          throw ChimeraTK::logic_error("FanOut::addSlave() has been called with a receiving implementation!");
        }
        // check if array shape is compatible, unless the receiver is a trigger node, so no data is expected
        if( slave->getNumberOfSamples() != 0 &&
            ( slave->getNumberOfChannels() != impl->getNumberOfChannels() ||
              slave->getNumberOfSamples() != impl->getNumberOfSamples()      ) ) {
          std::string what = "FanOut::addSlave(): Trying to add a slave '";
          what += slave->getName();
          what += "' with incompatible array shape! Name of master: ";
          what += impl->getName();
          throw ChimeraTK::logic_error(what.c_str());
        }
        slaves.push_back(slave);
      }

    protected:

      boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> impl;

      std::list<boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>>> slaves;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_FAN_OUT_H */