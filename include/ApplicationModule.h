/*
 * ApplicationModule.h
 *
 *  Created on: Jun 10, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_APPLICATION_MODULE_H
#define CHIMERATK_APPLICATION_MODULE_H

#include <list>

#include <boost/thread.hpp>

#include "Module.h"

namespace ChimeraTK {

  template< typename UserType >
  class Accessor;

  class AccessorBase;

  class ApplicationModule : public Module {

    public:

      /** Destructor */
      virtual ~ApplicationModule();

      /** To be implemented by the user: function called in a separate thread executing the main loop of the module */
      virtual void mainLoop() = 0;

      void run();

      void terminate();

    protected:

      template< typename UserType >
      friend class Accessor;

      friend class AccessorBase;

      /** Called inside the constructor of Accessor: adds the accessor to the list */
      void registerAccessor(AccessorBase* accessor) {
        accessorList.push_back(accessor);
      }

      /** The thread executing mainLoop() */
      boost::thread moduleThread;

      /** List of accessors owned by this module */
      std::list<AccessorBase*> accessorList;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_APPLICATION_MODULE_H */
