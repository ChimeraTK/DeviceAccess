/*
 * TwoDRegisterAccessor.h
 *
 *  Created on: Feb 3, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_TWO_DREGISTER_ACCESSOR_H
#define CHIMERA_TK_TWO_DREGISTER_ACCESSOR_H

#include <boost/smart_ptr.hpp>

#include "NDRegisterAccessorAbstractor.h"

namespace ChimeraTK {

  /** TODO add documentation
   */
  template<class UserType>
  class TwoDRegisterAccessor : public NDRegisterAccessorAbstractor<UserType> {

    public:

      /** Do not use this constructor directly. Instead call Device::getTwoDRegisterAccessor(). */
      TwoDRegisterAccessor( boost::shared_ptr< NDRegisterAccessor<UserType> > _accessor )
      : NDRegisterAccessorAbstractor<UserType>(_accessor)
      {}

      /** Placeholder constructer, to allow late initialisation of the accessor, e.g. in the open function.
       *  @attention Accessors created with this constructors will be dysfunctional, calling any member function
       *  will throw an exception (by the boost::shared_ptr)! */
      TwoDRegisterAccessor()
      {}

      /** Operator to access individual sequences/channels. */
      std::vector<UserType> & operator[](size_t channel) {
        return NDRegisterAccessorAbstractor<UserType>::_impl->accessChannel(channel);
      }

      /** Const operator to access individual sequences/channels. */
      const std::vector<UserType> & operator[](size_t channel) const {
        return NDRegisterAccessorAbstractor<UserType>::_impl->accessChannel(channel);
      }

      /** Return the number of channels (formerly called sequences) */
      size_t getNChannels() const {
        return NDRegisterAccessorAbstractor<UserType>::_impl->getNumberOfChannels();
      }

      /** Return number of elements/samples per channel */
      size_t getNElementsPerChannel() const {
        return NDRegisterAccessorAbstractor<UserType>::_impl->getNumberOfSamples();
      }

      /** DEPRECATED DO NOT USE
       *
       *  \deprecated
       *  This function is deprecated. Use getNChannels() instead! */
      size_t getNumberOfDataSequences() const {
        return NDRegisterAccessorAbstractor<UserType>::_impl->getNumberOfChannels();
      }

      /** DEPRECATED DO NOT USE
       *
       *  \deprecated
       *  This function is deprecated. Use getNChannels() instead! */
      size_t getNumberOfChannels() const {
        return NDRegisterAccessorAbstractor<UserType>::_impl->getNumberOfChannels();
      }

      /** DEPRECATED DO NOT USE
       *
       *  \deprecated
       *  This function is deprecated. Use getNElementsPerChannel() instead! */
      size_t getNumberOfSamples() const {
        return NDRegisterAccessorAbstractor<UserType>::_impl->getNumberOfSamples();
      }

      friend class TransferGroup;

  };

  // Do not declare the template for all user types as extern here.
  // This could avoid optimisation of the inline code.

} // namespace ChimeraTK

#endif /* CHIMERA_TK_TWO_DREGISTER_ACCESSOR_H */
