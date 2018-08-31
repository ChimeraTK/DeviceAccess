/*
 * BufferingRegisterAccessor.h
 *
 *  Created on: Sep 28, 2015
 *      Author: Martin Hierholzer <martin.hierholzer@desy.de>
 */

#ifndef CHIMERA_TK_BUFFERING_REGISTER_ACCESSOR_H
#define CHIMERA_TK_BUFFERING_REGISTER_ACCESSOR_H

#include "NDRegisterAccessorAbstractor.h"
#include "Exception.h"

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
        if(boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->getNumberOfChannels() != 1) {
          throw ChimeraTK::logic_error("The BufferingRegisterAccessor has a too low dimension to access this register.");
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
        return boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->accessData(0,index);
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      unsigned int getNumberOfElements() {
        return boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->getNumberOfSamples();
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      operator UserType&() {
        return boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->accessData(0,0);
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      BufferingRegisterAccessor<UserType>& operator=(UserType rightHandSide)
      {
        boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->accessData(0,0) = rightHandSide;
        return *this;
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      BufferingRegisterAccessor<UserType>& operator++() {
        return operator=( boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->accessData(0,0) + 1 );
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      BufferingRegisterAccessor<UserType>& operator--() {
        return operator=( boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->accessData(0,0) - 1 );
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      UserType operator++(int) {
        UserType v = boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->accessData(0,0);
        operator=( v + 1 );
        return v;
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      UserType operator--(int) {
        UserType v = boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->accessData(0,0);
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
      iterator begin() { return boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->accessChannel(0).begin(); }
      const_iterator begin() const { return boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->accessChannel(0).cbegin(); }
      const_iterator cbegin() const { return boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->accessChannel(0).cbegin(); }
      iterator end() { return boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->accessChannel(0).end(); }
      const_iterator end() const { return boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->accessChannel(0).cend(); }
      const_iterator cend() const { return boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->accessChannel(0).cend(); }
      reverse_iterator rbegin() { return boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->accessChannel(0).rbegin(); }
      const_reverse_iterator rbegin() const { return boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->accessChannel(0).crbegin(); }
      const_reverse_iterator crbegin() const { return boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->accessChannel(0).crbegin(); }
      reverse_iterator rend() { return boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->accessChannel(0).rend(); }
      const_reverse_iterator rend() const { return boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->accessChannel(0).crend(); }
      const_reverse_iterator crend() const { return boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->accessChannel(0).crend(); }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      void swap(std::vector<UserType> &x) {
        if(x.size() != boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->accessChannel(0).size()) {
          throw ChimeraTK::logic_error("Swapping with a buffer of a different size is not allowed.");
        }
        boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->accessChannel(0).swap(x);
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      bool isReadOnly() const {
        return boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->isReadOnly();
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      bool isReadable() const {
        return boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->isReadable();
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      bool isWriteable() const {
        return boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl)->isWriteable();
      }

      /** \brief DEPRECATED! Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  \deprecated
       *  This class is deprecated. Use OneDRegisterAccessor or ScalarRegisterAccessor instead!
       *  @todo Add printed runtime warning after release of version 0.9
       */
      bool isInitialised() const {
        return boost::static_pointer_cast<NDRegisterAccessor<UserType>>(_impl) != NULL;
      }

      friend class TransferGroup;

      using TransferElementAbstractor::_impl;

  };

}    // namespace ChimeraTK

#endif /* CHIMERA_TK_BUFFERING_REGISTER_ACCESSOR_H */
