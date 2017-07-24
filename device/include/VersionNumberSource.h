#ifndef CHIMERA_TK_VERSION_NUMBER_SOURCE_H
#define CHIMERA_TK_VERSION_NUMBER_SOURCE_H

#include <atomic>
#include <boost/shared_ptr.hpp>

#include "VersionNumber.h"

namespace ChimeraTK {

  /**
   * Source generating version numbers. Version numbers are used to resolve competing updates that are applied to the
   * same process variable. For example, it they can help in breaking an infinite update loop that might occur when
   * two process variables are related and update each other.
   * 
   * They are also used to determine the order of updates made to different process variables, e.g. to make sure that
   * TransferElement::readAny() always returns the oldest change first.
   * 
   * The class only has static members, so there is no need to create instances of this class.
   */
  class VersionNumberSource {

    public:

      /**
      * Returns the next version number. The next version number is determined in an atomic way, so that it is
      * guaranteed that this method never returns the same version number twice (unless the counter overflows, which
      * is very unlikely). The first version number returned by this method is one. The version number that is
      * returned is guaranteed to be greater than the version numbers returned for earlier calls to this method. This
      * method may safely be called by any thread without any synchronization.
      */
      static VersionNumber nextVersionNumber() {
        return ++_lastReturnedVersionNumber;
      }

    private:
      
      /** No instances needed. */
      VersionNumberSource();

      /**
      * Last version number that was returned by a call to
      * {@link nextVersionNumber()}.
      */
      static std::atomic<VersionNumber::UnderlyingDataType> _lastReturnedVersionNumber;

  };

}

#endif // CHIMERA_TK_VERSION_NUMBER_SOURCE_H
