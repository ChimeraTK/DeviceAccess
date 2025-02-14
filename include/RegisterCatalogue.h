// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "RegisterInfo.h"
#include "RegisterPath.h"

#include <boost/shared_ptr.hpp>

namespace ChimeraTK {

  class BackendRegisterCatalogueBase;
  class const_RegisterCatalogueImplIterator;

  /********************************************************************************************************************/

  /** Catalogue of register information */
  class RegisterCatalogue {
   public:
    explicit RegisterCatalogue(std::unique_ptr<BackendRegisterCatalogueBase>&& impl);

    RegisterCatalogue(const RegisterCatalogue& other);
    RegisterCatalogue(RegisterCatalogue&& other) noexcept;
    RegisterCatalogue& operator=(const RegisterCatalogue& other);
    RegisterCatalogue& operator=(RegisterCatalogue&& other) noexcept;

    ~RegisterCatalogue();

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

    /**
     * Return a const reference to the implementation object. Only for advanced use, e.g. when backend-depending code
     * shall be written.
     */
    [[nodiscard]] const BackendRegisterCatalogueBase& getImpl() const;

    /** Const iterator for iterating through the registers in the catalogue */
    class const_iterator {
     public:
      explicit const_iterator(std::unique_ptr<const_RegisterCatalogueImplIterator> it);

      const_iterator(const const_iterator& other);
      const_iterator& operator=(const const_iterator& other);

      const_iterator(const_iterator&& other) noexcept;
      const_iterator& operator=(const_iterator&& other) noexcept;

      const_iterator& operator++();
      const_iterator operator++(int);
      const_iterator& operator--();
      const_iterator operator--(int);
      const BackendRegisterInfoBase& operator*();
      const BackendRegisterInfoBase* operator->();
      bool operator==(const const_iterator& rightHandSide) const;
      bool operator!=(const const_iterator& rightHandSide) const;

     protected:
      std::unique_ptr<const_RegisterCatalogueImplIterator> _impl;
    };

    /** Return iterators for iterating through the registers in the catalogue */
    [[nodiscard]] const_iterator begin() const;
    [[nodiscard]] const_iterator end() const;

   protected:
    std::unique_ptr<BackendRegisterCatalogueBase> _impl;
  };

  /********************************************************************************************************************/

  /**
   * Virtual base class for the catalogue const iterator. The typical interator interface is realised in the
   * RegisterCatalogue::const_iterator class, which holds a pointer to this class (pimpl pattern).
   */
  class const_RegisterCatalogueImplIterator {
   public:
    virtual ~const_RegisterCatalogueImplIterator() = default;

    virtual void increment() = 0;

    virtual void decrement() = 0;

    [[nodiscard]] virtual const BackendRegisterInfoBase* get() = 0;

    [[nodiscard]] virtual bool isEqual(
        const std::unique_ptr<const_RegisterCatalogueImplIterator>& rightHandSide) const = 0;

    /**
     * Create copy of the iterator. This is required to implement the post-increment/decrement operators and
     * proper copy/assignment sematics of the RegisterCatalogue::const_iterator.
     */
    [[nodiscard]] virtual std::unique_ptr<const_RegisterCatalogueImplIterator> clone() const = 0;
  };

  /********************************************************************************************************************/

} /* namespace ChimeraTK */
