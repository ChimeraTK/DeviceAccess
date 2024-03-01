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
  class InterruptControllerHandler;
  class TriggeredPollDistributor;
  class TriggerDistributor;
  template<typename UserType>
  class VariableDistributor;
  class DeviceBackend;
  class AsyncDomain;

  /** Knows which type of InterruptControllerHandler to create for which interrupt.
   *  It is filled from the meta information from the map file.
   */
  class InterruptControllerHandlerFactory {
   public:
    explicit InterruptControllerHandlerFactory(DeviceBackend* backend);

    boost::shared_ptr<InterruptControllerHandler> createInterruptControllerHandler(
        std::vector<uint32_t> const& controllerID, boost::shared_ptr<TriggerDistributor> parent);
    void addControllerDescription(
        std::vector<uint32_t> const& controllerID, std::string const& name, std::string const& description);

    boost::shared_ptr<DeviceBackend> getBackend();

   protected:
    DeviceBackend* _backend;

    /** The key of this map is the controllerID.
     *  The value is a string pair of controller name and the description string from the map file.
     */
    std::map<std::vector<uint32_t>, std::pair<std::string, std::string>> _controllerDescriptions;

    /** Each controller type is registered via name and creator function.
     */
    std::map<std::string,
        std::function<std::unique_ptr<InterruptControllerHandler>(InterruptControllerHandlerFactory*,
            std::vector<uint32_t> const&, std::string, boost::shared_ptr<TriggerDistributor>)>>
        _creatorFunctions;
  };

  /** Interface base class for interrupt controller handlers. It implements the interface with the
   * DeviceBackend and the TriggerDistributors. Implementations must fill the pure virtual "handle()"
   * function with life and register the constructor to the factory.
   */
  class InterruptControllerHandler : public boost::enable_shared_from_this<InterruptControllerHandler> {
   public:
    /** InterruptControllerHandler classes must only be constructed inside and held by a DeviceBackend,
     * which is known to the handler via plain pointer (to avoid shared pointer loops)
     */
    InterruptControllerHandler(InterruptControllerHandlerFactory* controllerHandlerFactory,
        std::vector<uint32_t> controllerID, boost::shared_ptr<TriggerDistributor> parent);
    virtual ~InterruptControllerHandler() = default;

    /** Needed to get a new accessor for a certain interrupt. The whole chain will be created recursively if it does not
     * exist yet. The only valid DistrubutorTypes are TriggeredPollDistributor and VariableDistributor<std::nullptr_t>.
     */
    template<typename DistributorType>
    [[nodiscard]] boost::shared_ptr<DistributorType> getDistributorRecursive(std::vector<uint32_t> const& interruptID);

    void activate(VersionNumber version);
    void sendException(const std::exception_ptr& e);

    /** The interrupt handling functions implements the handshake with the interrupt controller. It needs to
     * be implemented individually for each interrupt controller.
     */
    virtual void handle(VersionNumber version) = 0;

   protected:
    std::map<uint32_t, boost::weak_ptr<TriggerDistributor>> _distributors;

    boost::shared_ptr<DeviceBackend> _backend;
    InterruptControllerHandlerFactory* _controllerHandlerFactory;

    /** The ID of this controller handler.
     */
    std::vector<uint32_t> _id;

    boost::shared_ptr<TriggerDistributor> _parent;
    boost::shared_ptr<AsyncDomain> _asyncDomain;

    /* These functions are needed so the compiler generates the template code. We cannot put the implementation into
     * this header because it needs full class descriptions, which would lead to circular header inclusions.
     * They are not called, but the template code is.
     */
    boost::shared_ptr<TriggeredPollDistributor> getPollDistributorRecursive(std::vector<uint32_t> const& interruptID);
    boost::shared_ptr<VariableDistributor<std::nullptr_t>> getVariableDistributorRecursive(
        std::vector<uint32_t> const& interruptID);
  };

} // namespace ChimeraTK
