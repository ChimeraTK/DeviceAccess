/*
 * TwoDRegisterAccessor.h
 *
 *  Created on: Feb 3, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_TWO_DREGISTER_ACCESSOR_H
#define MTCA4U_TWO_DREGISTER_ACCESSOR_H

#include <boost/smart_ptr.hpp>

#include "NDRegisterAccessorBridge.h"

namespace mtca4u {

  /** TODO add documentation
   */
  template<class UserType>
  class TwoDRegisterAccessor : public NDRegisterAccessorBridge<UserType> {

    public:

      /** Do not use this constructor directly. Instead call Device::getTwoDRegisterAccessor(). */
      TwoDRegisterAccessor( boost::shared_ptr< NDRegisterAccessor<UserType> > _accessor )
      : NDRegisterAccessorBridge<UserType>(_accessor)
      {}

      /** Placeholder constructer, to allow late initialisation of the accessor, e.g. in the open function.
       *  @attention Accessors created with this constructors will be dysfunctional, calling any member function
       *  will throw an exception (by the boost::shared_ptr)! */
      TwoDRegisterAccessor()
      {}

      /** Operator to access individual sequences/channels. */
      std::vector<UserType> & operator[](size_t channel) {
        return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(channel);
      }

      /** Const operator to access individual sequences/channels. */
      const std::vector<UserType> & operator[](size_t channel) const {
        return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(channel);
      }

      /** Return the number of channels (formerly called sequences) */
      size_t getNChannels() const {
        return NDRegisterAccessorBridge<UserType>::_impl->getNumberOfChannels();
      }

      /** Return number of elements/samples per channel */
      size_t getNElementsPerChannel() const {
        return NDRegisterAccessorBridge<UserType>::_impl->getNumberOfSamples();
      }

      /** DEPRECATED DO NOT USE
       *
       *  \deprecated
       *  This function is deprecated. Use getNChannels() instead! */
      size_t getNumberOfDataSequences() const {
        return NDRegisterAccessorBridge<UserType>::_impl->getNumberOfChannels();
      }

      /** DEPRECATED DO NOT USE
       *
       *  \deprecated
       *  This function is deprecated. Use getNChannels() instead! */
      size_t getNumberOfChannels() const {
        return NDRegisterAccessorBridge<UserType>::_impl->getNumberOfChannels();
      }

      /** DEPRECATED DO NOT USE
       *
       *  \deprecated
       *  This function is deprecated. Use getNElementsPerChannel() instead! */
      size_t getNumberOfSamples() const {
        return NDRegisterAccessorBridge<UserType>::_impl->getNumberOfSamples();
      }

      friend class TransferGroup;

  };

} // namespace mtca4u

#endif /* MTCA4U_TWO_DREGISTER_ACCESSOR_H */
