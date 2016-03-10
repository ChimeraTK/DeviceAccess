/*
 * RegisterCatalogue.h
 *
 *  Created on: Mar 8, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_REGISTER_CATALOGUE_H
#define MTCA4U_REGISTER_CATALOGUE_H

#include <map>

#include "RegisterInfo.h"
#include "RegisterPath.h"

namespace mtca4u {

  /** Catalogue of register information */
  class RegisterCatalogue {
  
    public:
    
      /** Get register information for a given full path name. */
      boost::shared_ptr<RegisterInfo> getRegister(const RegisterPath& registerPathName) const;
    
      /** Add register information to the catalogue. The full path name of the register is taken from the
       *  RegisterInfo structure. */
      void addRegister(boost::shared_ptr<RegisterInfo> registerInfo);

      /** Iterators for iterating through the registers in the catalogue */
      class const_iterator {
        public:
          const_iterator& operator++() {    // ++it
            ++iterator;
            return *this;
          }
          const_iterator operator++(int) { // it++
            const_iterator temp(*this);
            ++iterator;
            return temp;
          }
          const RegisterInfo& operator*() {
            return *(iterator->second);
          }
          boost::shared_ptr<RegisterInfo> operator->() {
            return iterator->second;
          }
          operator const boost::shared_ptr<RegisterInfo>&() {
            return iterator->second;
          }
          bool operator==(const const_iterator &rightHandSide) {
            return rightHandSide.iterator == iterator;
          }
          bool operator!=(const const_iterator &rightHandSide) {
            return rightHandSide.iterator != iterator;
          }
        protected:
          std::map< RegisterPath, boost::shared_ptr<RegisterInfo> >::const_iterator iterator;
          friend class RegisterCatalogue;
      };
      
      /** Return iterators for iterating through the registers in the catalogue */
      const_iterator begin() const {
        const_iterator it;
        it.iterator = catalogue.cbegin();
        return it;
      }
      const_iterator end() const {
        const_iterator it;
        it.iterator = catalogue.cend();
        return it;
      }
  
    protected:
    
      /** Map of register path names to register information. A more efficient way to store this information
       *  would be a tree-like structure, but this can be optimised later without an interface change. */
      std::map< RegisterPath, boost::shared_ptr<RegisterInfo> > catalogue;
  
  };


} /* namespace mtca4u */

#endif /* MTCA4U_REGISTER_CATALOGUE_H */
