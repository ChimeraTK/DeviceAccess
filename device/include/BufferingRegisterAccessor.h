/*
 * BufferingRegisterAccessor.h
 *
 *  Created on: Sep 28, 2015
 *      Author: Martin Hierholzer <martin.hierholzer@desy.de>
 */

#ifndef MTCA4U_BUFFERING_REGISTER_ACCESSOR_H
#define MTCA4U_BUFFERING_REGISTER_ACCESSOR_H

#include "NDRegisterAccessorAbstractor.h"
#include "DeviceException.h"

namespace ChimeraTK {

  /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
   *  \deprecated
   *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
   *  @todo Add printed runtime warning after release of version 0.9
   */
  template<typename UserType>
  class BufferingRegisterAccessor : public  NDRegisterAccessorAbstractor<UserType> {
    public:

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      BufferingRegisterAccessor(boost::shared_ptr< NDRegisterAccessor<UserType> > impl)
      : NDRegisterAccessorAbstractor<UserType>(impl)
      {
        if(NDRegisterAccessorAbstractor<UserType>::_impl->getNumberOfChannels() != 1) {
          throw DeviceException("The BufferingRegisterAccessor has a too low dimension to access this register.",
              DeviceException::WRONG_ACCESSOR);
        }
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      BufferingRegisterAccessor() {}

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      UserType& operator[](unsigned int index) {
        return NDRegisterAccessorAbstractor<UserType>::_impl->accessData(0,index);
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      unsigned int getNumberOfElements() {
        return NDRegisterAccessorAbstractor<UserType>::_impl->getNumberOfSamples();
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      operator UserType&() {
        return NDRegisterAccessorAbstractor<UserType>::_impl->accessData(0,0);
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      BufferingRegisterAccessor<UserType>& operator=(UserType rightHandSide)
      {
        NDRegisterAccessorAbstractor<UserType>::_impl->accessData(0,0) = rightHandSide;
        return *this;
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      BufferingRegisterAccessor<UserType>& operator++() {
        return operator=( NDRegisterAccessorAbstractor<UserType>::_impl->accessData(0,0) + 1 );
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      BufferingRegisterAccessor<UserType>& operator--() {
        return operator=( NDRegisterAccessorAbstractor<UserType>::_impl->accessData(0,0) - 1 );
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      UserType operator++(int) {
        UserType v = NDRegisterAccessorAbstractor<UserType>::_impl->accessData(0,0);
        operator=( v + 1 );
        return v;
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      UserType operator--(int) {
        UserType v = NDRegisterAccessorAbstractor<UserType>::_impl->accessData(0,0);
        operator=( v - 1 );
        return v;
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      typedef typename std::vector<UserType>::iterator iterator;
      typedef typename std::vector<UserType>::const_iterator const_iterator;
      typedef typename std::vector<UserType>::reverse_iterator reverse_iterator;
      typedef typename std::vector<UserType>::const_reverse_iterator const_reverse_iterator;
      iterator begin() { return NDRegisterAccessorAbstractor<UserType>::_impl->accessChannel(0).begin(); }
      const_iterator begin() const { return NDRegisterAccessorAbstractor<UserType>::_impl->accessChannel(0).cbegin(); }
      const_iterator cbegin() const { return NDRegisterAccessorAbstractor<UserType>::_impl->accessChannel(0).cbegin(); }
      iterator end() { return NDRegisterAccessorAbstractor<UserType>::_impl->accessChannel(0).end(); }
      const_iterator end() const { return NDRegisterAccessorAbstractor<UserType>::_impl->accessChannel(0).cend(); }
      const_iterator cend() const { return NDRegisterAccessorAbstractor<UserType>::_impl->accessChannel(0).cend(); }
      reverse_iterator rbegin() { return NDRegisterAccessorAbstractor<UserType>::_impl->accessChannel(0).rbegin(); }
      const_reverse_iterator rbegin() const { return NDRegisterAccessorAbstractor<UserType>::_impl->accessChannel(0).crbegin(); }
      const_reverse_iterator crbegin() const { return NDRegisterAccessorAbstractor<UserType>::_impl->accessChannel(0).crbegin(); }
      reverse_iterator rend() { return NDRegisterAccessorAbstractor<UserType>::_impl->accessChannel(0).rend(); }
      const_reverse_iterator rend() const { return NDRegisterAccessorAbstractor<UserType>::_impl->accessChannel(0).crend(); }
      const_reverse_iterator crend() const { return NDRegisterAccessorAbstractor<UserType>::_impl->accessChannel(0).crend(); }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      void swap(std::vector<UserType> &x) {
        if(x.size() != NDRegisterAccessorAbstractor<UserType>::_impl->accessChannel(0).size()) {
          throw DeviceException("Swapping with a buffer of a different size is not allowed.",
              DeviceException::WRONG_PARAMETER);
        }
        NDRegisterAccessorAbstractor<UserType>::_impl->accessChannel(0).swap(x);
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      bool isReadOnly() const {
        return NDRegisterAccessorAbstractor<UserType>::_impl->isReadOnly();
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      bool isReadable() const {
        return NDRegisterAccessorAbstractor<UserType>::_impl->isReadable();
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      bool isWriteable() const {
        return NDRegisterAccessorAbstractor<UserType>::_impl->isWriteable();
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      bool isInitialised() const {
        return NDRegisterAccessorAbstractor<UserType>::_impl != NULL;
      }

      friend class TransferGroup;
  };

}    // namespace ChimeraTK

#endif /* MTCA4U_BUFFERING_REGISTER_ACCESSOR_H */
