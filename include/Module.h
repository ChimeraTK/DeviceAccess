/*
 * Module.h
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_MODULE_H
#define CHIMERATK_MODULE_H

namespace ChimeraTK {
  
  template< typename UserType >
  class Accessor;
  
  class AccessorBase;
  
  /** Base class for ApplicationModule, DeviceModule and ControlSystemModule, to have a common interface for these
   *  module types. */
  class Module {

    public:

      /** Constructor: register the module with the Application */
      Module();

      /** Destructor */
      virtual ~Module() {}

      /** Prepare the execution of the module. */
      virtual void prepare() {};

      /** Execute the module. */
      virtual void run() {};

      /** Terminate the module. Must be called before destruction, if run() was called previously. */
      virtual void terminate() {};
      
      /** Obtain the list of accessors/variables associated with this module */
      const std::list<AccessorBase*>& getAccessorList() { return accessorList; }
      
  protected:
      
      template< typename UserType >
      friend class Accessor;
      
      friend class AccessorBase;
      
      /** Called inside the constructor of Accessor: adds the accessor to the list */
      void registerAccessor(AccessorBase* accessor) {
        accessorList.push_back(accessor);
      }
      
      /** List of accessors owned by this module */
      std::list<AccessorBase*> accessorList;
      
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_MODULE_H */
