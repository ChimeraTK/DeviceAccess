/*
 * RegisterCatalogue.h
 *
 *  Created on: Mar 8, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_REGISTER_CATALOGUE_H
#define CHIMERA_TK_REGISTER_CATALOGUE_H

#include <boost/shared_ptr.hpp>
#include <map>

#include "RegisterInfo.h"
#include "RegisterPath.h"

namespace ChimeraTK {

  /** Catalogue of register information */
  class RegisterCatalogue {
   public:
    /** Get register information for a given full path name. */
    boost::shared_ptr<RegisterInfoImpl> getRegister(const RegisterPath& registerPathName) const;

    /** Check if register with the given path name exists. */
    bool hasRegister(const RegisterPath& registerPathName) const;

    /** Get number of registers in the catalogue */
    size_t getNumberOfRegisters() const;

    /** Const iterator for iterating through the registers in the catalogue */
    class const_iterator {
     public:
      const_iterator() {}

      const_iterator(std::vector<RegisterInfo>::const_iterator theIterator_) : theIterator(theIterator_) {}

      const_iterator& operator++() { // ++it
        ++theIterator;
        return *this;
      }
      const_iterator operator++(int) { // it++
        const_iterator temp(*this);
        ++theIterator;
        return temp;
      }
      const_iterator& operator--() { // --it
        --theIterator;
        return *this;
      }
      const_iterator operator--(int) { // it--
        const_iterator temp(*this);
        --theIterator;
        return temp;
      }
      const RegisterInfo& operator*() { return *(theIterator->get()); }
      const RegisterInfo& operator->() { return boost::static_pointer_cast<const RegisterInfoImpl>(*theIterator); }
      const RegisterInfo& get() { return boost::static_pointer_cast<const RegisterInfoImpl>(*theIterator); }
      bool operator==(const const_iterator& rightHandSide) const { return rightHandSide.theIterator == theIterator; }
      bool operator!=(const const_iterator& rightHandSide) const { return rightHandSide.theIterator != theIterator; }

     protected:
      std::vector<RegisterInfo>::const_iterator theIterator;
    };

    /** Return iterators for iterating through the registers in the catalogue */
    const_iterator begin() const { return {catalogue.cbegin()}; }
    const_iterator end() const { return {catalogue.cend()}; }

   protected:
    /** Vector of RegisterInfo entries. A more efficient way to store this
     * information would be a tree-like structure, but this can be optimised later
     * without an interface change.
     *
     *  Note, to be compatible with the original RegisterInfoMap used for the
     * MemoryAddressedBackends the data storage must retain the order of elements
     * as they are added and allow duplicate entries. Both features are not
     * critical for a proper functioning, but dropping them is a small interface
     * change.
     *  */
    std::vector<boost::shared_ptr<RegisterInfoImpl>> catalogue;
  };

  /**
   */
  class MetadataCatalogue {
    /** Get metadata information for the given key. */
    const std::string& getMetadata(const std::string& key) const;

    /** Get number of metadata entries in the catalogue */
    size_t getNumberOfMetadata() const;

    /** Add metadata information to the catalogue. Metadata is stored as a
     * key=value pair of strings. */
    void addMetadata(const std::string& key, const std::string& value);

    /** Iterators for meta data */
    typedef std::map<std::string, std::string>::iterator iterator;
    typedef std::map<std::string, std::string>::const_iterator const_iterator;
    iterator begin();
    const_iterator begin() const;
    iterator end();
    const_iterator end() const;

   protected:
    /** Map of meta data */
    std::map<std::string, std::string> metadata;
  };

  /**
   * Interface for backends to the register catalogue. In addition to the functionality offered by the RegisterCatalogue
   * class, the content of the catalogue can be modified through this interface.
   */
  template<typename BackendRegisterInfo>
  class BackendRegisterCatalogue : RegisterCatalogue {
   public:
    /** Add register information to the catalogue. The full path name of the
     * register is taken from the RegisterInfo structure. */
    void addRegister(const boost::shared_ptr<BackendRegisterInfo>& registerInfo);

    void removeRegister(const RegisterPath& name);

    /** Non-const iterator for iterating through the registers in the catalogue */
    class iterator {
     public:
      iterator() {}

      iterator(typename std::vector<boost::shared_ptr<RegisterInfo>>::iterator theIterator_)
      : theIterator(theIterator_) {}

      iterator& operator++() { // ++it
        ++theIterator;
        return *this;
      }
      iterator operator++(int) { // it++
        iterator temp(*this);
        ++theIterator;
        return temp;
      }
      iterator& operator--() { // --it
        --theIterator;
        return *this;
      }
      iterator operator--(int) { // it--
        iterator temp(*this);
        --theIterator;
        return temp;
      }
      boost::shared_ptr<BackendRegisterInfo>& operator*() { return *(theIterator->get()); }
      boost::shared_ptr<BackendRegisterInfo>& operator->() { return *theIterator; }
      boost::shared_ptr<BackendRegisterInfo>& get() { return *theIterator; }
      operator const_iterator() { return {theIterator}; }
      bool operator==(const iterator& rightHandSide) const { return rightHandSide.theIterator == theIterator; }
      bool operator!=(const iterator& rightHandSide) const { return rightHandSide.theIterator != theIterator; }

     protected:
      typename std::vector<boost::shared_ptr<RegisterInfo>>::iterator theIterator;
    };

    /** Return iterators for iterating through the registers in the catalogue */
    iterator begin() { return {catalogue.begin()}; }
    iterator end() { return {catalogue.end()}; }
  };

} /* namespace ChimeraTK */

#endif /* CHIMERA_TK_REGISTER_CATALOGUE_H */
