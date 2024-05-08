// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "../DeviceBackend.h"
#include "../VersionNumber.h"
#include "TriggeredPollDistributor.h"
#include "VariableDistributor.h"

#include <boost/make_shared.hpp>

namespace ChimeraTK::async {
  class MuxedInterruptDistributorFactory;
  class MuxedInterruptDistributor;
  class Domain;

  template<typename UserType>
  class AsyncNDRegisterAccessor;

  namespace detail {
    template<typename UserType, typename BackendSpecificDataType>
    class SubDomainSubscriptionImplementor;
  } // namespace detail

  /********************************************************************************************************************/

  /** Send backend-specific asynchronous data to different distributors:
   *  \li MuxedInterruptDistributor
   *  \li TriggeredPollDistributor
   *  \li VariableDistributor<BackendSpecificDataType>
   */
  template<typename BackendSpecificDataType>
  class SubDomain : public boost::enable_shared_from_this<SubDomain<BackendSpecificDataType>> {
   public:
    SubDomain(boost::shared_ptr<DeviceBackend> backend, std::vector<size_t> qualifiedAsyncId,
        boost::shared_ptr<MuxedInterruptDistributor> parent, boost::shared_ptr<Domain> domain);

    void activate(BackendSpecificDataType, VersionNumber v);
    void distribute(BackendSpecificDataType, VersionNumber v);
    void sendException(const std::exception_ptr& e);

    /**
     * Get an AsyncAccessorManager for a specific SubDomain. The qualified SubDomain ID is relative to (and including)
     * this SubDomain. If the ID has a length 1, it will return the AsyncAccessorManager for the matching
     * DistributorType (TriggeredPollDistributor or VariableDistributor<BackendSpecificDataType>). If the ID is longer,
     * it will get the distributor from the matching SubDomain further down the hierarchy.
     * The Distributor and intermediate MuxedInterruptDistributors/SubDomains are created if they are not there.
     */
    template<typename DistributorType>
    boost::shared_ptr<AsyncAccessorManager> getAccessorManager(std::vector<size_t> const& qualifiedSubDomainId);

    boost::shared_ptr<Domain> getDomain() { return _domain; }
    std::vector<size_t> getId() { return _id; }
    boost::shared_ptr<DeviceBackend> getBackend() { return _backend; }

    template<typename UserType>
    boost::shared_ptr<AsyncNDRegisterAccessor<UserType>> subscribe(
        RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

   protected:
    std::vector<size_t> _id;

    boost::shared_ptr<DeviceBackend> _backend;
    boost::weak_ptr<MuxedInterruptDistributor> _muxedInterruptDistributor;
    boost::weak_ptr<TriggeredPollDistributor> _pollDistributor;
    boost::weak_ptr<VariableDistributor<std::nullptr_t>> _variableDistributor;
    boost::shared_ptr<MuxedInterruptDistributor> _parent;
    boost::shared_ptr<Domain> _domain;

    template<typename UserType, typename BackendDataType>
    friend class detail::SubDomainSubscriptionImplementor;
  };

  /********************************************************************************************************************/

  namespace detail {
    // Helper class to get instances for all user types. We cannot put the implementation into the header because of
    // circular header inclusion, and we cannot write a "for all user types" macro for functions because of the return
    // value and the function signature.
    template<typename UserType, typename BackendSpecificDataType>
    class SubDomainSubscriptionImplementor {
     public:
      static boost::shared_ptr<AsyncNDRegisterAccessor<UserType>> subscribeTo(
          SubDomain<BackendSpecificDataType>& subDomain, RegisterPath name, size_t numberOfWords,
          size_t wordOffsetInRegister, AccessModeFlags flags);
    };

  } // namespace detail

  /********************************************************************************************************************/

  template<typename BackendSpecificDataType>
  SubDomain<BackendSpecificDataType>::SubDomain(boost::shared_ptr<DeviceBackend> backend,
      std::vector<size_t> qualifiedAsyncId, boost::shared_ptr<MuxedInterruptDistributor> parent,
      boost::shared_ptr<Domain> domain)
  : _id(std::move(qualifiedAsyncId)), _backend(backend), _parent(std::move(parent)), _domain(std::move(domain)) {}

  /********************************************************************************************************************/
  template<typename BackendSpecificDataType>
  template<typename UserType>
  boost::shared_ptr<AsyncNDRegisterAccessor<UserType>> SubDomain<BackendSpecificDataType>::subscribe(
      RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    return detail::SubDomainSubscriptionImplementor<UserType, BackendSpecificDataType>::subscribeTo(
        *this, name, numberOfWords, wordOffsetInRegister, flags);
  }

  /********************************************************************************************************************/

  template<typename BackendSpecificDataType>
  template<typename DistributorType>
  boost::shared_ptr<AsyncAccessorManager> SubDomain<BackendSpecificDataType>::getAccessorManager(
      std::vector<size_t> const& qualifiedSubDomainId) {
    if(qualifiedSubDomainId.size() == 1) {
      // return the distributor from this instance, not a from a SubDomain further down the tree

      boost::weak_ptr<DistributorType>* weakDistributor{
          nullptr}; // Cannot create a reference here, but references in the "if constexpr" scope are not seen outside

      if constexpr(std::is_same<DistributorType, TriggeredPollDistributor>::value) {
        weakDistributor = &_pollDistributor;
      }
      else if constexpr(std::is_same<DistributorType, VariableDistributor<BackendSpecificDataType>>::value) {
        weakDistributor = &_variableDistributor;
      }
      else {
        throw ChimeraTK::logic_error("SubDomain::getAccessorManager(): Wrong template parameter.");
      }

      auto distributor = weakDistributor->lock();
      if(!distributor) {
        distributor = boost::make_shared<DistributorType>(_backend, this->shared_from_this(), _domain);
        *weakDistributor = distributor;
        if(_domain->unsafeGetIsActive()) {
          // Creating a new accessor in an activated domain is only supported if the BackendSpecificDataType is
          // nullptr_t. At the moment there are two use cases we need:
          //
          // 1. BackendSpecificDataType is nullptr_t.
          //    - There are three distributors (TriggeredPollDistributor, VariableDistributor<nullptr_t> and the
          //      InterruptControllerHandler) and a hierarchy of TriggerDistributors.
          //    - You can get an accessor to one of the distributors and receive data (active domain), and then a second
          //      distributor is created.
          // 2. The BackendSpecificDataType contains all required data.
          //    - There is no hierarchy of TriggerDistributors.
          //    - The VariableDistributor<BackendSpecificDataType> will be the only distributor and if it is not there,
          //      it means the domain has just been created and is not activated yet. As the VariableDistributor is
          //      holding the only ownership of the TriggerDistributor, both will go away together.
          //
          // At the moment the code does not support a combined option, which would require the option the get the
          // initial value for the newly created distributor here.
          if constexpr(std::is_same<BackendSpecificDataType, std::nullptr_t>::value) {
            // In case the BackendSpecificDataType is nullptr_t, we know
            // - the initial value is nullptr
            // - the version number cannot be determined from the data and we have to invent a new version number here
            distributor->distribute(nullptr, {});
          }
          else {
            // To put an implementation here, we need a way to get an initial value
            // (for instance from the domain, see https://redmine.msktools.desy.de/issues/13038).
            // If you run into this assertion, chances are that you accidentally ran into this code branch because the
            // domain has been activated too early due to a bug.
            assert(false);
          }
        }
      }
      return distributor;
    }
    // get a distributor from further down the tree, behind one or more MuxedInterruptDistributors
    auto muxedInterruptDistributor = _muxedInterruptDistributor.lock();
    if(!muxedInterruptDistributor) {
      muxedInterruptDistributor =
          MuxedInterruptDistributorFactory::getInstance().createMuxedInterruptDistributor(this->shared_from_this());
      _muxedInterruptDistributor = muxedInterruptDistributor;
      if(_domain->unsafeGetIsActive()) {
        // As for the distributor<nullptr_t> we activate using a new version number, as no version is strictly related
        // to the "data".
        muxedInterruptDistributor->activate({});
      }
    }

    return muxedInterruptDistributor->getAccessorManager<DistributorType>(
        {++qualifiedSubDomainId.begin(), qualifiedSubDomainId.end()});
  }

  /********************************************************************************************************************/

  template<typename BackendSpecificDataType>
  void SubDomain<BackendSpecificDataType>::distribute(BackendSpecificDataType data, VersionNumber version) {
    if(!_domain->unsafeGetIsActive()) {
      return;
    }
    auto pollDistributor = _pollDistributor.lock();
    if(pollDistributor) {
      pollDistributor->distribute(nullptr, version);
    }
    auto muxedInterruptDistributor = _muxedInterruptDistributor.lock();
    if(muxedInterruptDistributor) {
      muxedInterruptDistributor->handle(version);
    }
    auto variableDistributor = _variableDistributor.lock();
    if(variableDistributor) {
      variableDistributor->distribute(data, version);
    }
  }

  /********************************************************************************************************************/

  template<typename BackendSpecificDataType>
  void SubDomain<BackendSpecificDataType>::activate(BackendSpecificDataType data, VersionNumber version) {
    auto pollDistributor = _pollDistributor.lock();
    if(pollDistributor) {
      pollDistributor->distribute(nullptr, version);
    }
    auto muxedInterruptDidstributor = _muxedInterruptDistributor.lock();
    if(muxedInterruptDidstributor) {
      muxedInterruptDidstributor->activate(version);
    }
    auto variableDistributor = _variableDistributor.lock();
    if(variableDistributor) {
      variableDistributor->distribute(data, version);
    }
  }

  /********************************************************************************************************************/

  template<typename BackendSpecificDataType>
  void SubDomain<BackendSpecificDataType>::sendException(const std::exception_ptr& e) {
    auto pollDistributor = _pollDistributor.lock();
    if(pollDistributor) {
      pollDistributor->sendException(e);
    }
    auto muxedInterruptDistributor = _muxedInterruptDistributor.lock();
    if(muxedInterruptDistributor) {
      muxedInterruptDistributor->sendException(e);
    }
    auto variableDistributor = _variableDistributor.lock();
    if(variableDistributor) {
      variableDistributor->sendException(e);
    }
  }

  /********************************************************************************************************************/

  namespace detail {

    template<typename UserType, typename BackendSpecificDataType>
    boost::shared_ptr<AsyncNDRegisterAccessor<UserType>> SubDomainSubscriptionImplementor<UserType,
        BackendSpecificDataType>::subscribeTo(SubDomain<BackendSpecificDataType>& subDomain, RegisterPath name,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
      auto registerInfo = subDomain._backend->getRegisterCatalogue().getRegister(name);

      // Find the right place in the distribution tree to subscribe
      boost::shared_ptr<AsyncAccessorManager> distributor;
      if constexpr(std::is_same<BackendSpecificDataType, std::nullptr_t>::value) {
        // Special implementation for data type nullptr_t: Use a poll distributor if the data is not
        // FundamentalType::nodata itself
        if(registerInfo.getDataDescriptor().fundamentalType() == DataDescriptor::FundamentalType::nodata) {
          distributor = subDomain.template getAccessorManager<VariableDistributor<std::nullptr_t>>(
              registerInfo.getQualifiedAsyncId());
        }
        else {
          distributor =
              subDomain.template getAccessorManager<TriggeredPollDistributor>(registerInfo.getQualifiedAsyncId());
        }
      }
      else {
        // For all other BackendSpecificDataType use the according VariableDistributor.
        // This scheme might need some improvement later.
        distributor = subDomain.template getAccessorManager<VariableDistributor<BackendSpecificDataType>>(
            registerInfo.getQualifiedAsyncId());
      }

      return distributor->template subscribe<UserType>(name, numberOfWords, wordOffsetInRegister, flags);
    }

    INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(SubDomainSubscriptionImplementor, std::nullptr_t);
  } // namespace detail

  /********************************************************************************************************************/

} // namespace ChimeraTK::async
