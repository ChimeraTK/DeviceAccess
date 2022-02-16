/*
 * RegisterCatalogue.h
 *
 *  Created on: Mar 8, 2016
 *      Author: Martin Hierholzer
 */

#pragma once

#include <boost/shared_ptr.hpp>
#include <map>

#include "RegisterInfo.h"
#include "RegisterPath.h"

namespace ChimeraTK {

  class RegisterCatalogueImpl;
  class const_RegisterCatalogueImplIterator;

  /** Catalogue of register information */
  class RegisterCatalogue {
   public:
    /** 
     *  Get register information for a given full path name.
     *  
     *  Throws ChimeraTK::logic_error if register does not exist in the catalogue.
     */
    [[nodiscard]] RegisterInfo getRegister(const RegisterPath& registerPathName) const;

    /** Check if register with the given path name exists. */
    [[nodiscard]] bool hasRegister(const RegisterPath& registerPathName) const;

    /** Get number of registers in the catalogue */
    [[nodiscard]] size_t getNumberOfRegisters() const;

    /** Const iterator for iterating through the registers in the catalogue */
    class const_iterator {
     public:
      explicit const_iterator(boost::shared_ptr<const_RegisterCatalogueImplIterator> it);

      const_iterator(const const_iterator& other);
      const_iterator& operator=(const const_iterator& other);

      const_iterator(const_iterator&& other) noexcept;
      const_iterator& operator=(const_iterator&& other) noexcept;

      const_iterator& operator++();
      const_iterator operator++(int);
      const_iterator& operator--();
      const_iterator operator--(int);
      RegisterInfo operator*();
      RegisterInfo operator->();
      bool operator==(const const_iterator& rightHandSide) const;
      bool operator!=(const const_iterator& rightHandSide) const;

     protected:
      boost::shared_ptr<const_RegisterCatalogueImplIterator> impl;
    };

    /** Return iterators for iterating through the registers in the catalogue */
    [[nodiscard]] const_iterator cbegin() const;
    [[nodiscard]] const_iterator cend() const;

   protected:
    boost::shared_ptr<RegisterCatalogueImpl> impl;
  };

} /* namespace ChimeraTK */
