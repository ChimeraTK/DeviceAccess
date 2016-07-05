/*
 * BufferingRegisterAccessor.h
 *
 *  Created on: Sep 28, 2015
 *      Author: Martin Hierholzer <martin.hierholzer@desy.de>
 */

#ifndef MTCA4U_BUFFERING_REGISTER_ACCESSOR_H
#define MTCA4U_BUFFERING_REGISTER_ACCESSOR_H

#include "NDRegisterAccessorBridge.h"
#include "DeviceException.h"

namespace mtca4u {

  /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
   *  \deprecated
   *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
   *  @todo Add printed runtime warning after release of version 0.9
   */
  template<typename UserType>
  class BufferingRegisterAccessor : public  NDRegisterAccessorBridge<UserType> {
    public:

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      BufferingRegisterAccessor(boost::shared_ptr< NDRegisterAccessor<UserType> > impl)
      : NDRegisterAccessorBridge<UserType>(impl)
      {
        if(NDRegisterAccessorBridge<UserType>::_impl->getNumberOfChannels() != 1) {
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
      void read() {
        NDRegisterAccessorBridge<UserType>::_impl->read();
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      void write() {
        NDRegisterAccessorBridge<UserType>::_impl->write();
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      UserType& operator[](unsigned int index) {
        return NDRegisterAccessorBridge<UserType>::_impl->accessData(0,index);
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      unsigned int getNumberOfElements() {
        return NDRegisterAccessorBridge<UserType>::_impl->getNumberOfSamples();
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      operator UserType&() {
        return NDRegisterAccessorBridge<UserType>::_impl->accessData(0,0);
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      BufferingRegisterAccessor<UserType>& operator=(UserType rightHandSide)
      {
        NDRegisterAccessorBridge<UserType>::_impl->accessData(0,0) = rightHandSide;
        return *this;
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      BufferingRegisterAccessor<UserType>& operator++() {
        return operator=( NDRegisterAccessorBridge<UserType>::_impl->accessData(0,0) + 1 );
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      BufferingRegisterAccessor<UserType>& operator--() {
        return operator=( NDRegisterAccessorBridge<UserType>::_impl->accessData(0,0) - 1 );
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      UserType operator++(int) {
        UserType v = NDRegisterAccessorBridge<UserType>::_impl->accessData(0,0);
        operator=( v + 1 );
        return v;
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      UserType operator--(int) {
        UserType v = NDRegisterAccessorBridge<UserType>::_impl->accessData(0,0);
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
      iterator begin() { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).begin(); }
      const_iterator begin() const { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).cbegin(); }
      const_iterator cbegin() const { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).cbegin(); }
      iterator end() { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).end(); }
      const_iterator end() const { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).cend(); }
      const_iterator cend() const { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).cend(); }
      reverse_iterator rbegin() { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).rbegin(); }
      const_reverse_iterator rbegin() const { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).crbegin(); }
      const_reverse_iterator crbegin() const { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).crbegin(); }
      reverse_iterator rend() { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).rend(); }
      const_reverse_iterator rend() const { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).crend(); }
      const_reverse_iterator crend() const { return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).crend(); }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      void swap(std::vector<UserType> &x) {
        NDRegisterAccessorBridge<UserType>::_impl->accessChannel(0).swap(x);
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      bool isReadOnly() const {
        return NDRegisterAccessorBridge<UserType>::_impl->isReadOnly();
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      bool isReadable() const {
        return NDRegisterAccessorBridge<UserType>::_impl->isReadable();
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      bool isWriteable() const {
        return NDRegisterAccessorBridge<UserType>::_impl->isWriteable();
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      bool isInitialised() const {
        return NDRegisterAccessorBridge<UserType>::_impl != NULL;
      }

      friend class TransferGroup;
  };

}    // namespace mtca4u

#endif /* MTCA4U_BUFFERING_REGISTER_ACCESSOR_H */
