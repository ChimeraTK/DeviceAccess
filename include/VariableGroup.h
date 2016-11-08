/*
 * VariableGroup.h
 *
 *  Created on: Nov 8, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_VARIABLE_GROUP_H
#define CHIMERATK_VARIABLE_GROUP_H

#include <list>

#include <boost/thread.hpp>

#include "Module.h"

namespace ChimeraTK {

  class VariableGroup : public Module {

    public:

      /** Destructor */
      virtual ~VariableGroup();
      
      /** Wait for receiving an update for any of the push-type variables in the group. Any poll-type variables are
       *  read after receiving the update. If no push-type variables are in the group, this function will just read
       *  all variables. */
      void readAny();

    protected:

      /** The thread executing mainLoop() */
      boost::thread moduleThread;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_VARIABLE_GROUP_H */
