/*
 * RegisterCatalogue.h
 *
 *  Created on: Mar 8, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_REGISTER_CATALOGUE_H
#define CHIMERA_TK_REGISTER_CATALOGUE_H

#include <map>

#include "RegisterInfo.h"
#include "RegisterPath.h"

namespace ChimeraTK {

  /** Catalogue of register information */
  class RegisterCatalogue {

    public:

      /** Get register information for a given full path name. */
      boost::shared_ptr<RegisterInfo> getRegister(const RegisterPath &registerPathName) const;

      /** Check if register with the given path name exists. */
      bool hasRegister(const RegisterPath &registerPathName) const;

      /** Get metadata information for the given key. */
      const std::string& getMetadata(const std::string &key) const;

      /** Get number of registers in the catalogue */
      size_t getNumberOfRegisters() const;

      /** Get number of metadata entries in the catalogue */
      size_t getNumberOfMetadata() const;

      /** Add register information to the catalogue. The full path name of the register is taken from the
       *  RegisterInfo structure. */
      void addRegister(boost::shared_ptr<RegisterInfo> registerInfo);

      /** Add metadata information to the catalogue. Metadata is stored as a key=value pair of strings. */
      void addMetadata(const std::string &key, const std::string &value);

      /** Const iterator for iterating through the registers in the catalogue */
      class const_iterator {

        public:

          const_iterator() {}

          const_iterator(std::vector< boost::shared_ptr<RegisterInfo> >::const_iterator theIterator_)
          : theIterator(theIterator_) {}

          const_iterator& operator++() {    // ++it
            ++theIterator;
            return *this;
          }
          const_iterator operator++(int) { // it++
            const_iterator temp(*this);
            ++theIterator;
            return temp;
          }
          const_iterator& operator--() {    // --it
            --theIterator;
            return *this;
          }
          const_iterator operator--(int) { // it--
            const_iterator temp(*this);
            --theIterator;
            return temp;
          }
          const RegisterInfo& operator*() {
            return *(theIterator->get());
          }
          boost::shared_ptr<const RegisterInfo> operator->() {
            return boost::static_pointer_cast<const RegisterInfo>(*theIterator);
          }
          boost::shared_ptr<const RegisterInfo> get() {
            return boost::static_pointer_cast<const RegisterInfo>(*theIterator);
          }
          bool operator==(const const_iterator &rightHandSide) const {
            return rightHandSide.theIterator == theIterator;
          }
          bool operator!=(const const_iterator &rightHandSide) const {
            return rightHandSide.theIterator != theIterator;
          }

        protected:

          std::vector< boost::shared_ptr<RegisterInfo> >::const_iterator theIterator;

      };

      /** Return iterators for iterating through the registers in the catalogue */
      const_iterator begin() const {
        return {catalogue.cbegin()};
      }
      const_iterator end() const {
        return {catalogue.cend()};
      }

      /** Non-const iterator for iterating through the registers in the catalogue */
      class iterator {

        public:

          iterator() {}

          iterator(std::vector< boost::shared_ptr<RegisterInfo> >::iterator theIterator_)
          : theIterator(theIterator_) {}

          iterator& operator++() {    // ++it
            ++theIterator;
            return *this;
          }
          iterator operator++(int) { // it++
            iterator temp(*this);
            ++theIterator;
            return temp;
          }
          iterator& operator--() {    // --it
            --theIterator;
            return *this;
          }
          iterator operator--(int) { // it--
            iterator temp(*this);
            --theIterator;
            return temp;
          }
          RegisterInfo& operator*() {
            return *(theIterator->get());
          }
          boost::shared_ptr<RegisterInfo> operator->() {
            return *theIterator;
          }
          const boost::shared_ptr<RegisterInfo>& get() {
            return *theIterator;
          }
          operator const_iterator() {
            return {theIterator};
          }
          bool operator==(const iterator &rightHandSide) const {
            return rightHandSide.theIterator == theIterator;
          }
          bool operator!=(const iterator &rightHandSide) const {
            return rightHandSide.theIterator != theIterator;
          }

      protected:

          std::vector< boost::shared_ptr<RegisterInfo> >::iterator theIterator;

      };

      /** Return iterators for iterating through the registers in the catalogue */
      iterator begin() {
        return {catalogue.begin()};
      }
      iterator end() {
        return {catalogue.end()};
      }

      /** Iterators for meta data */
      typedef std::map< std::string, std::string >::iterator metadata_iterator;
      typedef std::map< std::string, std::string >::const_iterator metadata_const_iterator;
      metadata_iterator metadata_begin();
      metadata_const_iterator metadata_begin() const;
      metadata_iterator metadata_end();
      metadata_const_iterator metadata_end() const;

    protected:

      /** Vector of RegisterInfo entries. A more efficient way to store this information
       *  would be a tree-like structure, but this can be optimised later without an interface change.
       *
       *  Note, to be compatible with the original RegisterInfoMap used for the MemoryAddressedBackends the data
       *  storage must retain the order of elements as they are added and allow duplicate entries. Both features
       *  are not critical for a proper functioning, but dropping them is a small interface change.
       *  */
      std::vector< boost::shared_ptr<RegisterInfo> > catalogue;

      /** Map of meta data */
      std::map< std::string, std::string > metadata;

  };


} /* namespace ChimeraTK */

#endif /* CHIMERA_TK_REGISTER_CATALOGUE_H */
