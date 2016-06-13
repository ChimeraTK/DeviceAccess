/*
 * PublishedAccessor.h
 *
 *  Created on: Jun 10, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_PUBLISHED_ACCESSOR_H
#define CHIMERATK_PUBLISHED_ACCESSOR_H

#include "Accessor.h"

namespace ChimeraTK {

  /** A PublishedAccessor is used in place of an accessor in the Application's variable lists where a publication to
   *  the control system adapter is to be made. The PublishedAccessor *cannot* be used as a normal accessor, instead
   *  it represents the control-system-side end of a process variable in the Application's variable lists. Once all
   *  connections and publications are made, the variable lists and thus the PublishedAccessors are no longer needed
   *  and can be destroyed.
   *
   *  @todo TODO To make it more clear, change the control system adapter's PVManager to accept outside-created
   *  process variables instead forcing us to use its createProcessArray/Scalar() function. This would make the
   *  handling of published variables more symmetric to the other variables. */
  template< typename UserType >
  class PublishedAccessor : public Accessor<UserType> {
    public:

      /** Construct the PublishedAccessor placeholder. The given name will be used to identify the variable in the
       *  control system. The direction specifies the variable's direction from the point-of-view of the control
       *  system, i.e. an output variable will be control-system-to-device. */
      PublishedAccessor(const std::string& name, VariableDirection direction, std::string unit)
      : Accessor<UserType>(nullptr, name, direction, unit, UpdateMode::push)
      {

        // convert direction. Note: The "direction" variable in this context is the direction from the control system's
        // point-of-view!
        SynchronizationDirection synchronizationDirection;
        if(direction == VariableDirection::output) {
          synchronizationDirection = SynchronizationDirection::controlSystemToDevice;
          std::cout << "Creating process variable: " << name << " as controlSystemToDevice" << std::endl;
        }
        else {
          synchronizationDirection = SynchronizationDirection::deviceToControlSystem;
          std::cout << "Creating process variable: " << name << " as deviceToControlSystem" << std::endl;
        }

        // create process variable
        impl = Application::getInstance().getPVManager()->createProcessScalar<UserType>(synchronizationDirection, name);
      }

      boost::shared_ptr<ProcessVariable> createProcessVariable() {
        return impl;
      }

      void useProcessVariable(boost::shared_ptr<ProcessVariable> &) {
        throw std::string("useProcessVariable() may not be used for PublishedAccessors."); // @todo TODO throw proper exception
      }

      bool isInitialised() const { return true; }

    protected:

      boost::shared_ptr< ProcessScalar<UserType> > impl;
  };


} /* namespace ChimeraTK */

#endif /* CHIMERATK_PUBLISHED_ACCESSOR_H */
