/*
 * SyncNDRegisterAccessor.h
 *
 *  Created on: Nov 22, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_SYNC_N_D_REGISTER_ACCESSOR_H
#define CHIMERA_TK_SYNC_N_D_REGISTER_ACCESSOR_H

#include <boost/make_shared.hpp>

#include "ForwardDeclarations.h"
#include "NDRegisterAccessor.h"
#include "TransferElement.h"

namespace ChimeraTK {

  /** NDRegisterAccessor for backends with only synchronous transfers. It checks that AccessMode::wait_for_new_data
   * is not set and throws a ChimeraTK::logic_error in the constructor if it is set. */
  template<typename UserType>
  class SyncNDRegisterAccessor : public NDRegisterAccessor<UserType> {
   public:
    SyncNDRegisterAccessor(std::string const& name, AccessModeFlags accessModeFlags,
        std::string const& unit = std::string(TransferElement::unitNotSet),
        std::string const& description = std::string())
    : NDRegisterAccessor<UserType>(name, accessModeFlags, unit, description) {
      if(accessModeFlags.has(AccessMode::wait_for_new_data)) {
        throw ChimeraTK::logic_error(
            "TransferElement for " + name + " does not support AccessMode::wait_for_new_data.");
      }
    }

    ~SyncNDRegisterAccessor() {
      // This is a requirement to all implementations: call shutdown() in the
      // destructor!
      assert(shutdownCalled);
    }

    /**
     * All implementations must call this function in their destructor. Also,
     * implementations must call it in their constructors before throwing an
     * exception (hint: put catch-all block around the entired constructor, call
     * shutdown() there and then rethrow the exception).
     *
     * Implementation note: This function call is necessary to ensure that a
     * potentially still-running thread launched in readAsync() is properly
     * terminated before destroying the accessor object. Since this thread
     * accesses virtual functions like doReadTransfer(), the full accessor object
     * must still be alive, thus shutting down the thread in the base class
     * destructor is too late. Technically, implementations overriding readAsync()
     * would not need to call this function, but to make sure all implementations
     * which do not override readAsync() actually call it, the function call is
     * enforced for all implementations in the destructor with an assert.
     */
    void shutdown() { shutdownCalled = true; }

   private:
    /// Flag whether shutdown() has been called or not
    bool shutdownCalled{false};
  };

} // namespace ChimeraTK

#endif /* CHIMERA_TK_SYNC_N_D_REGISTER_ACCESSOR_H */
