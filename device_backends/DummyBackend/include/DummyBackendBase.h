/**
 * @file DummyBackendBase.h
 *
 * Common definitions and base class for DummyBackends
 */
#pragma once

#include "NumericAddressedBackend.h"
#include "NumericAddressedBackendRegisterAccessor.h"
#include "NumericAddressedBackendMuxedRegisterAccessor.h"
#include "AsyncNDRegisterAccessor.h"
#include "NumericAddressedInterruptDispatcher.h"
#include "DummyInterruptTriggerAccessor.h"

#include <sstream>
#include <regex>

// macro to avoid code duplication
#define TRY_REGISTER_ACCESS(COMMAND)                                                                                   \
  try {                                                                                                                \
    COMMAND                                                                                                            \
  }                                                                                                                    \
  catch(std::out_of_range & outOfRangeException) {                                                                     \
    std::stringstream errorMessage;                                                                                    \
    errorMessage << "Invalid address offset " << address << " in bar " << bar << "."                                   \
                 << "Caught out_of_range exception: " << outOfRangeException.what();                                   \
    throw ChimeraTK::logic_error(errorMessage.str());                                                                  \
  }                                                                                                                    \
  while(false)

namespace ChimeraTK {
  /**
   * Base class for DummyBackends, provides common functionality
   */
  class DummyBackendBase : public NumericAddressedBackend {
   protected:
    DummyBackendBase(std::string const& mapFileName);
    DummyBackendBase(std::string const& mapFileName) : NumericAddressedBackend(mapFileName) {

    ~DummyBackendBase() override;

    size_t minimumTransferAlignment([[maybe_unused]] uint64_t bar) const override;

    /** Simulate the arrival of an interrupt. For all push-type accessors which have been created
     *  for that particular interrupt controller and interrupt number, the data will be read out
     *  through a synchronous accessor and pushed into the data transport queues of the asynchronous
     *  accessors, so they can be received by the application.
     *
     *   @returns The version number that was send with all data in this interrupt.
     */
    virtual VersionNumber triggerInterrupt(int interruptControllerNumber, int interruptNumber) = 0;

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
      for(auto const& info : _registerMap) {
        barSizesInBytes[info.bar] =
            std::max(barSizesInBytes[info.bar], static_cast<size_t>(info.address + info.nBytes));

    static void checkSizeIsMultipleOfWordSize(size_t sizeInBytes);

    /// Specific override which allows to create "DUMMY_WRITEABLE" accessors for read-only registers
    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(const RegisterPath& registerPathName,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
      // First check if the request is for one of the special DUMMY_INTEERRUPT_X_Y registers. if so, early return
      // this special accessor.
      // Pseudo-register to trigger interrupts for this register
      static const std::string DUMMY_INTERRUPT_REGISTER_NAME{"^/DUMMY_INTERRUPT_([0-9]+)_([0-9]+)$"};

      const std::string regPathNameStr{registerPathName};
      const std::regex dummyInterruptRegex{DUMMY_INTERRUPT_REGISTER_NAME};
      std::smatch match;
      std::regex_search(regPathNameStr, match, dummyInterruptRegex);

      if(not match.empty()) {
        // FIXME: Ideally, this test and the need for passing in the lambda function should be done
        // in the constructor of the accessor. But passing down the base-class of the backend is very weird
        // due to the sort-of CRTP pattern used in this base class.
        auto controller = std::stoi(match[1].str());
        auto interrupt = std::stoi(match[2].str());
        try {
          auto& interruptsForController = _registerMap->getListOfInterrupts().at(controller);
          if(interruptsForController.find(interrupt) == interruptsForController.end())
            throw ChimeraTK::logic_error("Invalid interrupt for controller (" + match[0].str() + ", " +
                match[1].str() + ": " + regPathNameStr);
        }
        catch(std::out_of_range&) {
          throw ChimeraTK::logic_error("Invalid interrupt controller (" + match[0].str() + ", " +
              match[1].str() + ": " + regPathNameStr);
        }

        // Delegate the other parameters down to the accessor which will throw accordingly, to satisfy the specification
        // Since the accessor will keep a shared pointer to the backend, we can safely capture "this"
        auto d = new DummyInterruptTriggerAccessor<UserType>(
            shared_from_this(), [this, controller, interrupt]() { return triggerInterrupt(controller, interrupt); },
            registerPathName, numberOfWords, wordOffsetInRegister, flags);

        return boost::shared_ptr<NDRegisterAccessor<UserType>>(d);
      }

      // Suffix to mark writeable references to read-only registers
      // This is just a special case of a "normal" register, so can be handled together with getting the regular accessor.
      static const std::string DUMMY_WRITEABLE_SUFFIX{".DUMMY_WRITEABLE"};
      bool isDummyWriteableAccessor = false;
      RegisterPath actualRegisterPath{registerPathName};

      // Check if register name ends on DUMMY_WRITEABLE_SUFFIX,
      // in that case, set actual path to the "real" register
      // which exists in the catalogue.
      //std::smatch match;
      const std::regex re{DUMMY_WRITEABLE_SUFFIX + "$"};
      std::regex_search(regPathNameStr, match, re);

      if(!match.empty()) {
        isDummyWriteableAccessor = true;
        actualRegisterPath = RegisterPath{match.prefix()};
      }

      auto accessor = NumericAddressedBackend::getRegisterAccessor_impl<UserType>(
          actualRegisterPath, numberOfWords, wordOffsetInRegister, flags);

      // Modify write-ability of the synchronous NumericAddressedBackendRegisterAccessor
      if(isDummyWriteableAccessor) {
        // the accessor might be synchronous or asynchronous. If it is an async accessor we have to add a dummy-writeable accessor
        boost::shared_ptr<NDRegisterAccessor<UserType>> syncAccessor;
        if(flags.has(AccessMode::wait_for_new_data)) {
          auto syncFlags = flags;
          syncFlags.remove(AccessMode::wait_for_new_data);
          syncAccessor = NumericAddressedBackend::getRegisterAccessor_impl<UserType>(
              actualRegisterPath, numberOfWords, wordOffsetInRegister, syncFlags);
          // we cannot add the new sync accessor to the async accessor as writeAccessor here, because it is not writeable yet
        }
        else {
          syncAccessor = accessor;
        }

        const auto info{getRegisterInfo(actualRegisterPath)};

        if(info.dataType == NumericAddressedRegisterInfo::Type::FIXED_POINT) {
          if(flags.has(AccessMode::raw)) {
            boost::dynamic_pointer_cast<NumericAddressedBackendRegisterAccessor<UserType, FixedPointConverter, true>>(
                syncAccessor)
                ->makeWriteable();
          }
          else {
            if(info.getNumberOfDimensions() < 2) {
              boost::dynamic_pointer_cast<
                  NumericAddressedBackendRegisterAccessor<UserType, FixedPointConverter, false>>(syncAccessor)
                  ->makeWriteable();
            }
            else {
              boost::dynamic_pointer_cast<NumericAddressedBackendMuxedRegisterAccessor<UserType>>(syncAccessor)
                  ->makeWriteable();
            }
          }
        }
        else if(info.dataType == NumericAddressedRegisterInfo::Type::IEEE754) {
          if(flags.has(AccessMode::raw)) {
            boost::dynamic_pointer_cast<
                NumericAddressedBackendRegisterAccessor<UserType, IEEE754_SingleConverter, true>>(syncAccessor)
                ->makeWriteable();
          }
          else {
            boost::dynamic_pointer_cast<
                NumericAddressedBackendRegisterAccessor<UserType, IEEE754_SingleConverter, false>>(syncAccessor)
                ->makeWriteable();
          }
        }

        if(flags.has(AccessMode::wait_for_new_data)) {
          // We still have to set the now writeable synchronous accessor in the asynchronous accessor to enable writing
          boost::dynamic_pointer_cast<AsyncNDRegisterAccessor<UserType>>(accessor)->setWriteAccessor(syncAccessor);
        }
      }

      return accessor;
    }

    /**
     * @brief Backward compatibility: Leftover from the time when the dummy managed it's own map
     * to return the same instance multiple times, and still allow to use the same map file with different instances.
     *
     * This functionality is now in the BackendFactory and has been removed here.
     * The function is still in here because existing backend implementations use it in their createInstance() functions.
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

} //namespace ChimeraTK