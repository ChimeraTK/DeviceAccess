/**
 * @file DummyBackendBase.h
 *
 * Common definitions and base class for DummyBackends
 */
#ifndef CHIMERA_TK_DUMMY_BACKEND_BASE_H
#define CHIMERA_TK_DUMMY_BACKEND_BASE_H

#include "NumericAddressedBackend.h"
#include "NumericAddressedBackendRegisterAccessor.h"
#include "NumericAddressedBackendMuxedRegisterAccessor.h"

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
   * Base class for DummyBackends, provides common funtionality
   *
   * Note: This is implemented as a CRTP because we need to access
   *       the static getInstanceMap() of the derived backends.
   */
  template<typename DerivedBackendType>
  class DummyBackendBase : public NumericAddressedBackend {
   private:
    // ctor & dtor private with derived type as friend to enforce
    // correct specialization
    friend DerivedBackendType;
    DummyBackendBase(std::string const& mapFileName)
    : NumericAddressedBackend(mapFileName), _registerMapping{_registerMap} {
      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
    }

    virtual ~DummyBackendBase() {}

    size_t minimumTransferAlignment() const override { return 4; }

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

   protected:
    RegisterInfoMapPointer _registerMapping;

    /// Determines the size of each bar because the DummyBackends allocate memory per bar
    std::map<uint64_t, size_t> getBarSizesInBytesFromRegisterMapping() const {
      std::map<uint64_t, size_t> barSizesInBytes;
      for(RegisterInfoMap::const_iterator mappingElementIter = _registerMapping->begin();
          mappingElementIter != _registerMapping->end(); ++mappingElementIter) {
        barSizesInBytes[mappingElementIter->bar] = std::max(barSizesInBytes[mappingElementIter->bar],
            static_cast<size_t>(mappingElementIter->address + mappingElementIter->nBytes));
      }
      return barSizesInBytes;
    }

    static void checkSizeIsMultipleOfWordSize(size_t sizeInBytes) {
      if(sizeInBytes % sizeof(int32_t)) {
        throw ChimeraTK::logic_error("Read/write size has to be a multiple of 4");
      }
    }

    /// Specific override which allows to create "DUMMY_WRITEABLE" accessors for read-only registers
    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(const RegisterPath& registerPathName,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
      // Suffix to mark writeable references to read-only registers
      static const std::string DUMMY_WRITEABLE_SUFFIX{".DUMMY_WRITEABLE"};

      bool isDummyWriteableAccessor = false;
      RegisterPath actualRegisterPath{registerPathName};

      // Check if register name ends on DUMMY_WRITEABLE_SUFFIX,
      // in that case, set actual path to the "real" register
      // which exists in the catalogue.
      const std::string regPathNameStr{registerPathName};
      std::smatch match;
      const std::regex re{DUMMY_WRITEABLE_SUFFIX + "$"};
      std::regex_search(regPathNameStr, match, re);

      if(!match.empty()) {
        isDummyWriteableAccessor = true;
        actualRegisterPath = RegisterPath{match.prefix()};
      }

      auto accessor = NumericAddressedBackend::getRegisterAccessor_impl<UserType>(
          actualRegisterPath, numberOfWords, wordOffsetInRegister, flags);

      // Modify writeability of the NumericAddressedBackendRegisterAccessor
      if(isDummyWriteableAccessor) {
        const auto info{getRegisterInfo(actualRegisterPath)};

        if(info->dataType == RegisterInfoMap::RegisterInfo::Type::FIXED_POINT) {
          if(flags.has(AccessMode::raw)) {
            boost::dynamic_pointer_cast<NumericAddressedBackendRegisterAccessor<UserType, FixedPointConverter, true>>(
                accessor)
                ->makeWriteable();
          }
          else {
            if(info->getNumberOfDimensions() < 2) {
              boost::dynamic_pointer_cast<
                  NumericAddressedBackendRegisterAccessor<UserType, FixedPointConverter, false>>(accessor)
                  ->makeWriteable();
            }
            else {
              boost::dynamic_pointer_cast<NumericAddressedBackendMuxedRegisterAccessor<UserType>>(accessor)
                  ->makeWriteable();
            }
          }
        }
        else if(info->dataType == RegisterInfoMap::RegisterInfo::Type::IEEE754) {
          if(flags.has(AccessMode::raw)) {
            boost::dynamic_pointer_cast<
                NumericAddressedBackendRegisterAccessor<UserType, IEEE754_SingleConverter, true>>(accessor)
                ->makeWriteable();
          }
          else {
            boost::dynamic_pointer_cast<
                NumericAddressedBackendRegisterAccessor<UserType, IEEE754_SingleConverter, false>>(accessor)
                ->makeWriteable();
          }
        }
      }

      accessor->setExceptionBackend(shared_from_this());
      return accessor;
    }

    /**
     * @brief Method looks up and returns an existing instance of class 'T'
     * corresponding to instanceId, if instanceId is a valid  key in the
     * internal map. For an instanceId not in the internal map, a new instance
     * of class T is created, cached and returned. Future calls to
     * returnInstance with this instanceId, returns this cached instance. If
     * the instanceId is "" a new instance of class T is created and
     * returned. This instance will not be cached in the internal memory.
     *
     * @param instanceId Used as key for the object instance look up. "" as
     *                   instanceId will return a new T instance that is not
     *                   cached.
     * @param arguments  This is a template argument list. The constructor of
     *                   the created class T, gets called with the contents of
     *                   the argument list as parameters.
     */
    template<typename T, typename... Args>
    static boost::shared_ptr<DeviceBackend> returnInstance(const std::string& instanceId, Args&&... arguments) {
      if(instanceId == "") {
        // std::forward because template accepts forwarding references
        // (Args&&) and this can have both lvalue and rvalue references passed
        // as arguments.
        return boost::shared_ptr<DeviceBackend>(new T(std::forward<Args>(arguments)...));
      }
      auto instanceMap = DerivedBackendType::getInstanceMap();

      // search instance map and create new instanceId, if not found under the
      // name
      if(DerivedBackendType::getInstanceMap().find(instanceId) == DerivedBackendType::getInstanceMap().end()) {
        boost::shared_ptr<DeviceBackend> ptr(new T(std::forward<Args>(arguments)...));
        DerivedBackendType::getInstanceMap().insert(std::make_pair(instanceId, ptr));
        return ptr;
      }
      // return existing instanceId from the map
      return boost::shared_ptr<DeviceBackend>(DerivedBackendType::getInstanceMap()[instanceId]);
    }

  }; // class DummyBackendBase

} //namespace ChimeraTK

#endif // CHIMERA_TK_DUMMY_BACKEND_BASE_H
