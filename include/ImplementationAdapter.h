/*
 * ImplementationAdapter.h
 *
 *  Created on: Jun 16, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_IMPLEMENTATION_ADAPTER_H
#define CHIMERATK_IMPLEMENTATION_ADAPTER_H

#include <thread>

#include <ChimeraTK/ControlSystemAdapter/ProcessArray.h>

namespace ChimeraTK {

  /** Simple base class just to be able to put the FanOuts into a list.
   *  @todo TODO find a better name! */
  class ImplementationAdapterBase {
    public:
      virtual ~ImplementationAdapterBase(){}

      /** Activate synchronisation thread if needed */
      virtual void activate() {}

      /** Deactivate synchronisation thread if running*/
      virtual void deactivate() {}
  };


} /* namespace ChimeraTK */

#endif /* CHIMERATK_IMPLEMENTATION_ADAPTER_H */
