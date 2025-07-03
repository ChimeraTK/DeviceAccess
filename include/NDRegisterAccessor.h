// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "Exception.h"
#include "ForwardDeclarations.h"
#include "TransferElement.h"
#include "VirtualFunctionTemplate.h"

#include <boost/container/static_vector.hpp>
#include <boost/make_shared.hpp>

namespace ChimeraTK {

  /**
   * N-dimensional register accessor. Base class for all register accessor implementations. The user frontend classes
   * ScalarRegisterAccessor, OneDRegisterAccessor, TwoDRegisterAccessor and VoidRegisterAccessor are using
   * implementations based on this class to perform the actual IO.
   */
  template<user_type UserType>
  class NDRegisterAccessor : public TransferElement {
   public:
    /** Creates an NDRegisterAccessor with the specified name (passed on to the
     *  transfer element). */
    NDRegisterAccessor(std::string const& name, AccessModeFlags accessModeFlags,
        std::string const& unit = std::string(TransferElement::unitNotSet),
        std::string const& description = std::string())
    : TransferElement(name, accessModeFlags, unit, description) {
      TransferElement::makeUniqueId();
      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAsCooked_impl);
      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(setAsCooked_impl);
    }

    /** Get or set register accessor's buffer content (1D version).
     *  @attention No bounds checking is performed, use getNumberOfSamples() to
     * obtain the number of elements in the register. */
    UserType& accessData(size_t sample) { return buffer_2D[0][sample]; }
    const UserType& accessData(size_t sample) const { return buffer_2D[0][sample]; }

    /** Get or set register accessor's buffer content (2D version).
     *  @attention No bounds checking is performed, use getNumberOfChannels() and
     * getNumberOfSamples() to obtain the number of channels and samples in the
     * register. */
    UserType& accessData(unsigned int channel, unsigned int sample) { return buffer_2D[channel][sample]; }
    const UserType& accessData(unsigned int channel, unsigned int sample) const { return buffer_2D[channel][sample]; }

    /** Get or set register accessor's channel vector.
     *  @attention No bounds checking is performed, use getNumberOfChannels() to
     * obtain the number of elements in the register. */
    std::vector<UserType>& accessChannel(unsigned int channel) { return buffer_2D[channel]; }
    const std::vector<UserType>& accessChannel(unsigned int channel) const { return buffer_2D[channel]; }

    /** Get or set register accessor's 2D channel vector. */
    std::vector<std::vector<UserType>>& accessChannels() { return buffer_2D; }
    const std::vector<std::vector<UserType>>& accessChannels() const { return buffer_2D; }

    /** Return number of elements per channel */
    unsigned int getNumberOfSamples() const { return buffer_2D[0].size(); }

    /** Return number of channels */
    unsigned int getNumberOfChannels() const { return buffer_2D.size(); }

    const std::type_info& getValueType() const override { return typeid(UserType); }

    template<typename COOKED_TYPE>
    COOKED_TYPE getAsCooked(unsigned int channel, unsigned int sample) const;
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAsCooked_impl, T const(unsigned int, unsigned int));
    template<typename COOKED_TYPE>
    COOKED_TYPE getAsCooked_impl(unsigned int channel, unsigned int sample) const;

    template<typename COOKED_TYPE>
    void setAsCooked(unsigned int channel, unsigned int sample, COOKED_TYPE value);
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(setAsCooked_impl, void(unsigned int, unsigned int, T));
    template<typename COOKED_TYPE>
    void setAsCooked_impl(unsigned int channel, unsigned int sample, COOKED_TYPE value);

    boost::shared_ptr<TransferElement> makeCopyRegisterDecorator() override;

    /**
     * @brief Decorate the innermost TransferElement of the stack of decorators or decorator-like accessors.
     *
     * Decorators (and certain decorator-like accessors which shall allow this type of "inside" decoration) shall
     * first attempt to delegate a call to decorateDeepInside() to their target. Accessors which cannot decorate an
     * internal target simply do not implement this function, so the default implementation returns a nulled shared_ptr.
     * Only if a decorator (or decorator-like accessor) sees that the delegated call returns a nulled shared_ptr, it
     * shall use the factory to decorate its target, and then return its new (now decorated) target. If the delegated
     * call returned a non-nulled shared_ptr, that shared_ptr must be passed through unaltered.
     *
     * @param[in] factory Functor object to create the decorator. The functor takes one argument, which is the target
     * accessor to be decorated. The functor may return a nulled pointer if the target is not suitable for decoration,
     * in which case it will be retired on level further out, if applicable.
     *
     * @return {nullptr} if no decoration can be done inside, otherwise the decorator created by the factory function.
     */
    [[nodiscard]] virtual boost::shared_ptr<NDRegisterAccessor<UserType>> decorateDeepInside(
        [[maybe_unused]] std::function<boost::shared_ptr<NDRegisterAccessor<UserType>>(
            const boost::shared_ptr<NDRegisterAccessor<UserType>>&)> factory) {
      return {};
    }

    /**
     * Data type to create individual buffers. They are mainly used in asynchronous
     * implementation. Each buffer stores a vector, the version
     * number and the time stamp. The type is swappable by the default
     * implementation of std::swap since both the move constructor and the move
     * assignment operator are implemented. This helps to avoid unnecessary memory
     * allocations when transported in a cppext::future_queue.
     */
    struct Buffer {
      Buffer(size_t nChannels, size_t nElements) : value(nChannels) {
        for(auto& channel : value) channel.resize(nElements);
      }

      Buffer() = default;

      Buffer(Buffer&& other) noexcept
      : value(std::move(other.value)), versionNumber(other.versionNumber), dataValidity(other.dataValidity) {}

      Buffer& operator=(Buffer&& other) noexcept {
        value = std::move(other.value);
        versionNumber = other.versionNumber;
        dataValidity = other.dataValidity;
        return *this;
      }

      /** The actual data contained in this buffer. */
      std::vector<std::vector<UserType>> value;

      /** Version number of this data */
      ChimeraTK::VersionNumber versionNumber{nullptr};

      /** Whether or not the data in the buffer is considered valid */
      ChimeraTK::DataValidity dataValidity{ChimeraTK::DataValidity::ok};
    };

   protected:
    /** Buffer of converted data elements. The buffer is always two dimensional.
     * If a register with a single dimension should be accessed, the outer vector
     * has only a single element. For a scalar register, only a single element is
     * present in total (buffer_2D[0][0]). This has a negligible performance
     * impact when optimisations are enabled, but allows a coherent interface for
     * all accessors independent of their dimension.
     *
     *  Implementation note: The buffer must be created with the right number of
     * elements in the constructor! */
    std::vector<std::vector<UserType>> buffer_2D;
    // boost::container::vector<boost::container::vector<UserType>> buffer_2D;

    /// the compatibility layers need access to the buffer_2D
    friend class RegisterAccessor;

    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(NDRegisterAccessor<UserType>, getAsCooked_impl, 2);
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(NDRegisterAccessor<UserType>, setAsCooked_impl, 3);
  };

  template<user_type UserType>
  template<typename COOKED_TYPE>
  COOKED_TYPE NDRegisterAccessor<UserType>::getAsCooked(unsigned int channel, unsigned int sample) const {
    return CALL_VIRTUAL_FUNCTION_TEMPLATE(getAsCooked_impl, COOKED_TYPE, channel, sample);
  }

  template<user_type UserType>
  template<typename COOKED_TYPE>
  COOKED_TYPE NDRegisterAccessor<UserType>::getAsCooked_impl(unsigned int /*channel*/, unsigned int /*sample*/) const {
    throw ChimeraTK::logic_error("Reading as cooked is not available for this accessor");
  }

  template<user_type UserType>
  template<typename COOKED_TYPE>
  void NDRegisterAccessor<UserType>::setAsCooked(unsigned int channel, unsigned int sample, COOKED_TYPE value) {
    CALL_VIRTUAL_FUNCTION_TEMPLATE(setAsCooked_impl, COOKED_TYPE, channel, sample, value);
  }

  template<user_type UserType>
  template<typename COOKED_TYPE>
  void NDRegisterAccessor<UserType>::setAsCooked_impl(
      unsigned int /*channel*/, unsigned int /*sample*/, COOKED_TYPE /*value*/) {
    throw ChimeraTK::logic_error("Setting as cooked is not available for this accessor");
  }

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NDRegisterAccessor);

} /* namespace ChimeraTK */
