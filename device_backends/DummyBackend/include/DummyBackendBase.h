// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AsyncNDRegisterAccessor.h"
#include "DummyBackendRegisterCatalogue.h"
#include "DummyInterruptTriggerAccessor.h"
#include "NumericAddressedBackend.h"
#include "NumericAddressedBackendMuxedRegisterAccessor.h"
#include "NumericAddressedBackendRegisterAccessor.h"

#include <sstream>

// macro to avoid code duplication
#define TRY_REGISTER_ACCESS(COMMAND)                                                                                   \
  try {                                                                                                                \
    COMMAND                                                                                                            \
  }                                                                                                                    \
  catch(std::out_of_range & outOfRangeException) {                                                                     \
    std::stringstream errorMessage;                                                                                    \
    errorMessage << "Invalid address offset " << address << " in bar " << bar << "."                                   \
                 << "Caught out_of_range exception: " << outOfRangeException.what();                                   \
    std::cout << errorMessage.str() << std::endl;                                                                      \
    throw ChimeraTK::logic_error(errorMessage.str());                                                                  \
  }

namespace ChimeraTK {
  /**
   * Base class for DummyBackends, provides common functionality
   */
  class DummyBackendBase : public NumericAddressedBackend {
   public:
    explicit DummyBackendBase(std::string const& mapFileName);

    ~DummyBackendBase() override = default;

    size_t minimumTransferAlignment([[maybe_unused]] uint64_t bar) const override;

    /** Simulate the arrival of an interrupt. For all push-type accessors which have been created
     *  for that particular interrupt number, the data will be read out
     *  through a synchronous accessor and pushed into the data transport queues of the asynchronous
     *  accessors, so they can be received by the application.
     *
     *   @returns The version number that was send with all data in this interrupt.
     */
    virtual VersionNumber triggerInterrupt(uint32_t interruptNumber) = 0;

    /** You cannot override the read version with 32 bit address any more. Please change your
     *  implementation to the 64 bit signature.
     */
    void read([[maybe_unused]] uint8_t bar, [[maybe_unused]] uint32_t address, [[maybe_unused]] int32_t* data,
        [[maybe_unused]] size_t sizeInBytes) final {
      throw;
    }

    /** You cannot override the write version with 32 bit address any more. Please change your
     *  implementation to the 64 bit signature.
     */
    void write([[maybe_unused]] uint8_t bar, [[maybe_unused]] uint32_t address, [[maybe_unused]] int32_t const* data,
        [[maybe_unused]] size_t sizeInBytes) final {
      throw;
    }

    /// All bars are valid in dummies.
    bool barIndexValid([[maybe_unused]] uint64_t bar) override;

    /// Determines the size of each bar because the DummyBackends allocate memory per bar
    std::map<uint64_t, size_t> getBarSizesInBytesFromRegisterMapping() const;

    static void checkSizeIsMultipleOfWordSize(size_t sizeInBytes);

    std::atomic<bool> throwExceptionOpen{false};
    std::atomic<bool> throwExceptionRead{false};
    std::atomic<bool> throwExceptionWrite{false};
    std::atomic<size_t> throwExceptionCounter{0};

    /// Specific override which allows to create "DUMMY_INTEERRUPT_X" accessors
    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(const RegisterPath& registerPathName,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
      // First check if the request is for one of the special DUMMY_INTEERRUPT_X registers. if so, early return
      // this special accessor.
      if(registerPathName.startsWith("DUMMY_INTERRUPT_")) {
        bool interruptFound;
        uint32_t interrupt;

        auto* dummyCatalogue = dynamic_cast<DummyBackendRegisterCatalogue*>(_registerMapPointer.get());
        assert(dummyCatalogue);
        std::tie(interruptFound, interrupt) = dummyCatalogue->extractControllerInterrupt(registerPathName);
        if(!interruptFound) {
          throw ChimeraTK::logic_error("Unknown dummy interrupt " + registerPathName);
        }

        // Delegate the other parameters down to the accessor which will throw accordingly, to satisfy the specification
        // Since the accessor will keep a shared pointer to the backend, we can safely capture "this"
        auto d = new DummyInterruptTriggerAccessor<UserType>(
            shared_from_this(), [this, interrupt]() { return triggerInterrupt(interrupt); }, registerPathName,
            numberOfWords, wordOffsetInRegister, flags);

        return boost::shared_ptr<NDRegisterAccessor<UserType>>(d);
      }

      return NumericAddressedBackend::getRegisterAccessor_impl<UserType>(
          registerPathName, numberOfWords, wordOffsetInRegister, flags);
    }

    /**
     * @brief Backward compatibility: Leftover from the time when the dummy managed it's own map
     * to return the same instance multiple times, and still allow to use the same map file with different instances.
     *
     * This functionality is now in the BackendFactory and has been removed here.
     * The function is still in here because existing backend implementations use it in their createInstance()
     * functions.
     *
     * @param instanceId Used as key for the object instance look up. "" as
     *                   instanceId will return a new T instance that is not
     *                   cached.
     * @param arguments  This is a template argument list. The constructor of
     *                   the created class T, gets called with the contents of
     *                   the argument list as parameters.
     */
    template<typename T, typename... Args>
    static boost::shared_ptr<DeviceBackend> returnInstance(
        [[maybe_unused]] const std::string& instanceId, Args&&... arguments) {
      return boost::make_shared<T>(std::forward<Args>(arguments)...);
    }

  }; // class DummyBackendBase

} // namespace ChimeraTK
