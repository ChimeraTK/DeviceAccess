/*
 * NDRegisterAccessor.h - N-dimensional register accessor
 *
 *  Created on: Mar 21, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_N_D_REGISTER_ACCESSOR_H
#define CHIMERA_TK_N_D_REGISTER_ACCESSOR_H

#include <boost/make_shared.hpp>

#include "Exception.h"
#include "ForwardDeclarations.h"
#include "TransferElement.h"
#include "VirtualFunctionTemplate.h"

namespace ChimeraTK {

  /** N-dimensional register accessor. Base class for all register accessor
   * implementations. The user frontend classes BufferingRegisterAccessor and
   * TwoDRegisterAccessor are using implementations based on this class to perform
   *  the actual IO. */
  template<typename UserType>
  class NDRegisterAccessor : public TransferElement {
   public:
    /** Creates an NDRegisterAccessor with the specified name (passed on to the
     *  transfer element). */
    NDRegisterAccessor(std::string const& name, std::string const& unit = std::string(TransferElement::unitNotSet),
        std::string const& description = std::string())
    : TransferElement(name, unit, description) {
      TransferElement::makeUniqueId();
      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAsCooked_impl);
      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(setAsCooked_impl);
    }

    /** Get or set register accessor's buffer content (1D version).
     *  @attention No bounds checking is performed, use getNumberOfSamples() to
     * obtain the number of elements in the register. */
    UserType& accessData(unsigned int sample) { return buffer_2D[0][sample]; }
    const UserType& accessData(unsigned int sample) const { return buffer_2D[0][sample]; }

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

    /// the compatibility layers need access to the buffer_2D
    friend class MultiplexedDataAccessor<UserType>;
    friend class RegisterAccessor;

    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(NDRegisterAccessor<UserType>, getAsCooked_impl, 2);
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(NDRegisterAccessor<UserType>, setAsCooked_impl, 3);
  };

  template<typename UserType>
  template<typename COOKED_TYPE>
  COOKED_TYPE NDRegisterAccessor<UserType>::getAsCooked(unsigned int channel, unsigned int sample) const {
    return CALL_VIRTUAL_FUNCTION_TEMPLATE(getAsCooked_impl, COOKED_TYPE, channel, sample);
  }

  template<typename UserType>
  template<typename COOKED_TYPE>
  COOKED_TYPE NDRegisterAccessor<UserType>::getAsCooked_impl(unsigned int /*channel*/, unsigned int /*sample*/) const {
    throw ChimeraTK::logic_error("Reading as cooked is not available for this accessor");
  }

  template<typename UserType>
  template<typename COOKED_TYPE>
  void NDRegisterAccessor<UserType>::setAsCooked(unsigned int channel, unsigned int sample, COOKED_TYPE value) {
    CALL_VIRTUAL_FUNCTION_TEMPLATE(setAsCooked_impl, COOKED_TYPE, channel, sample, value);
  }

  template<typename UserType>
  template<typename COOKED_TYPE>
  void NDRegisterAccessor<UserType>::setAsCooked_impl(
      unsigned int /*channel*/, unsigned int /*sample*/, COOKED_TYPE /*value*/) {
    throw ChimeraTK::logic_error("Setting as cooked is not available for this accessor");
  }

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NDRegisterAccessor);

} /* namespace ChimeraTK */

#endif /* CHIMERA_TK_N_D_REGISTER_ACCESSOR_H */
