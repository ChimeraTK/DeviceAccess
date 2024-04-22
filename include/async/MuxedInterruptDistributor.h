// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "VersionNumber.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace ChimeraTK {
  class DeviceBackend;
} // namespace ChimeraTK

namespace ChimeraTK::async {
  class MuxedInterruptDistributor;
  class AsyncAccessorManager;
  template<typename BackendSpecificDataType>
  class SubDomain;
  class Domain;

  /********************************************************************************************************************/

  class MuxedInterruptDistributorFactory {
   public:
    static MuxedInterruptDistributorFactory& getInstance();
    boost::shared_ptr<MuxedInterruptDistributor> createMuxedInterruptDistributor(
        boost::shared_ptr<SubDomain<std::nullptr_t>> parent);

   protected:
    /** Each controller type is registered via name and creator function.
     */
    std::map<std::string,
        std::function<std::unique_ptr<MuxedInterruptDistributor>(
            std::string, boost::shared_ptr<SubDomain<std::nullptr_t>>)>>
        _creatorFunctions;
    static std::unique_ptr<MuxedInterruptDistributorFactory> _instance;

    static std::pair<std::string, std::string> getInterruptControllerNameAndDescriptionFromCatalogue(
        std::vector<size_t> const& subdomainID, DeviceBackend& backend);

   private:
    MuxedInterruptDistributorFactory();
  };

  /********************************************************************************************************************/

  /** Interface base class for interrupt controller handlers. It implements the interface with the
   * DeviceBackend and the SubDomains. Implementations must fill the pure virtual "handle()"
   * function with life and register the constructor to the factory.
   */
  class MuxedInterruptDistributor : public boost::enable_shared_from_this<MuxedInterruptDistributor> {
   public:
    /** MuxedInterruptDistributor classes must only be constructed inside and held by a DeviceBackend,
     * which is known to the handler via plain pointer (to avoid shared pointer loops)
     */
    explicit MuxedInterruptDistributor(boost::shared_ptr<SubDomain<std::nullptr_t>> parent);
    virtual ~MuxedInterruptDistributor() = default;

    /**
     * Get an AsyncAccessorManager of type DistributorType from the matching SubDomain.
     * The qualififedSubDomainId is relative to this MuxedInterruptDistributor.
     * The SubDomain and the distributor are created if they don't exist.
     */
    template<typename DistributorType>
    [[nodiscard]] boost::shared_ptr<AsyncAccessorManager> getAccessorManager(
        std::vector<size_t> const& qualififedSubDomainId);

    void activate(VersionNumber version);
    void sendException(const std::exception_ptr& e);

    /** The interrupt handling functions implements the handshake with the interrupt controller. It needs to
     * be implemented individually for each interrupt controller.
     */
    virtual void handle(VersionNumber version) = 0;

   protected:
    std::map<size_t, boost::weak_ptr<SubDomain<std::nullptr_t>>> _subDomains;

    boost::shared_ptr<DeviceBackend> _backend;

    /** The ID of this controller handler.
     */
    std::vector<size_t> _id;

    boost::shared_ptr<SubDomain<std::nullptr_t>> _parent;
    boost::shared_ptr<Domain> _asyncDomain;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK::async
