// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "RegisterCatalogue.h"

#include <boost/make_shared.hpp>

namespace ChimeraTK {

  template<typename BackendRegisterInfo>
  class const_BackendRegisterCatalogueImplIterator;

  template<typename BackendRegisterInfo>
  class BackendRegisterCatalogueImplIterator;

  /********************************************************************************************************************/
  /* Register catalogue container class definitions ******************************************************************/
  /********************************************************************************************************************/

  /** Pure virtual implementation base class for the register catalogue. */
  class BackendRegisterCatalogueBase {
   public:
    BackendRegisterCatalogueBase() = default;

    BackendRegisterCatalogueBase(BackendRegisterCatalogueBase&& other) = default;

    BackendRegisterCatalogueBase& operator=(BackendRegisterCatalogueBase&& other) = default;

    virtual ~BackendRegisterCatalogueBase() = default;

    // Copy constructor/assignment is deleted. It could be implemented (need to take care of pointers in
    // insertionOrderedCatalogue), but currently there is no need for this and hence it is not done.
    BackendRegisterCatalogueBase(const BackendRegisterCatalogueBase& other) = delete;
    BackendRegisterCatalogueBase& operator=(const BackendRegisterCatalogueBase& other) = delete;

    /**
     *  Get register information for a given full path name.
     *
     *  Throws ChimeraTK::logic_error if register does not exist in the catalogue.
     */
    [[nodiscard]] virtual RegisterInfo getRegister(const RegisterPath& registerPathName) const = 0;

    /** Check if register with the given path name exists. */
    [[nodiscard]] virtual bool hasRegister(const RegisterPath& registerPathName) const = 0;

    /** Get number of registers in the catalogue */
    [[nodiscard]] virtual size_t getNumberOfRegisters() const = 0;

    /** Return begin iterator for iterating through the registers in the catalogue */
    [[nodiscard]] virtual std::unique_ptr<const_RegisterCatalogueImplIterator> getConstIteratorBegin() const = 0;

    /** Return end iterator for iterating through the registers in the catalogue */
    [[nodiscard]] virtual std::unique_ptr<const_RegisterCatalogueImplIterator> getConstIteratorEnd() const = 0;

    /** Create deep copy of the catalogue */
    [[nodiscard]] virtual std::unique_ptr<BackendRegisterCatalogueBase> clone() const = 0;
  };

  /********************************************************************************************************************/

  /**
   * Interface for backends to the register catalogue. In addition to the functionality offered by the RegisterCatalogue
   * class, the content of the catalogue can be modified through this interface.
   *
   * Backend implementations should instantiate this class with their backend-specific implementation of the
   * RegisterInfoImpl class.
   */
  template<typename BackendRegisterInfo>
  class BackendRegisterCatalogue : public BackendRegisterCatalogueBase {
   public:
    /// Note: Override this function if backend has "hidden" registers which are not added to the map and hence do not
    /// appear when iterating. Do not forget to also override hasRegister() in this case.
    [[nodiscard]] virtual BackendRegisterInfo getBackendRegister(const RegisterPath& registerPathName) const;

    [[nodiscard]] bool hasRegister(const RegisterPath& registerPathName) const override;

    /// Note: Implementation internally uses getBackendRegister(), so no need to override
    [[nodiscard]] RegisterInfo getRegister(const RegisterPath& registerPathName) const final;

    [[nodiscard]] size_t getNumberOfRegisters() const override;

    [[nodiscard]] std::unique_ptr<const_RegisterCatalogueImplIterator> getConstIteratorBegin() const override;

    [[nodiscard]] std::unique_ptr<const_RegisterCatalogueImplIterator> getConstIteratorEnd() const override;

    [[nodiscard]] std::unique_ptr<BackendRegisterCatalogueBase> clone() const override;

    /**
     * Add register information to the catalogue. The full path name of the register is taken from the RegisterInfo
     * structure.
     */
    void addRegister(const BackendRegisterInfo& registerInfo);

    /**
     * Remove register as identified by the given name from the catalogue. Throws ChimeraTK::logic_error if register
     * does not exist in the catalogue.
     */
    void removeRegister(const RegisterPath& name);

    /**
     * Replaces the register information for the matching register.
     * The full path name of the register to be modified is taken from the RegisterInfo
     * structure. You cannot change the name of the register.
     *
     * Throws ChimeraTK::logic_error if register
     * does not exist in the catalogue.
     */
    void modifyRegister(const BackendRegisterInfo& registerInfo);

    /** Return begin iterator for iterating through the registers in the catalogue */
    [[nodiscard]] BackendRegisterCatalogueImplIterator<BackendRegisterInfo> begin() {
      return BackendRegisterCatalogueImplIterator<BackendRegisterInfo>{insertionOrderedCatalogue.begin()};
    }

    /** Return end iterator for iterating through the registers in the catalogue */
    [[nodiscard]] BackendRegisterCatalogueImplIterator<BackendRegisterInfo> end() {
      return BackendRegisterCatalogueImplIterator<BackendRegisterInfo>{insertionOrderedCatalogue.end()};
    }

    /** Return const begin iterators for iterating through the registers in the catalogue */
    [[nodiscard]] const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo> begin() const {
      return const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>{insertionOrderedCatalogue.begin()};
    }

    /** Return const end iterators for iterating through the registers in the catalogue */
    [[nodiscard]] const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo> end() const {
      return const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>{insertionOrderedCatalogue.end()};
    }

    /** Return const begin iterators for iterating through the registers in the catalogue */
    [[nodiscard]] const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo> cbegin() const {
      return const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>{insertionOrderedCatalogue.begin()};
    }

    /** Return const end iterators for iterating through the registers in the catalogue */
    [[nodiscard]] const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo> cend() const {
      return const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>{insertionOrderedCatalogue.end()};
    }

   protected:
    /** Helper function for clone functions. It copies/clones the content of the private variables of the
     *  BackendRegisterCatalogue into the target catalogue. See implementation of the clone() function.
     */
    void fillFromThis(BackendRegisterCatalogue<BackendRegisterInfo>* target) const;

   private:
    // Always access the catalogue through the member functions. Modifications need special care to keep the two
    // containers synchronised, hence these members are made private.
    std::map<RegisterPath, BackendRegisterInfo> catalogue;
    std::vector<BackendRegisterInfo*> insertionOrderedCatalogue;
  };

  /********************************************************************************************************************/
  /* Iterator class definitions **************************************************************************************/
  /********************************************************************************************************************/

  /**
   * Implementation of the catalogue const iterator which is templated to the actual BackendRegisterInfo implementation
   * provided by the backend. Backends may use different implelementations of the iterator in case hooks are required
   * in the iterator functions (e.g. to implement lazy catalogue filling).
   *
   * Note: If a backend implements lazy catalogue filling, it can get away with hooks only inside the
   * BackendRegisterInfo implementations, as long as the list of register names is determined beforehand. Backends which
   * also lazy-fill the list of names should still put the filling of the register properties (besides the name) into
   * the BackendRegisterInfo implementation, since applications may obtain that directly via
   * RegisterCatalogue::getRegister() by the name.
   *
   * Implementation note: The reason for using a fully virtual iterator is not primarily to allow planting hooks into
   * the iterator. It is necessary since the map to iterate holds the backend-specific BackendRegisterInfo type, hence
   * it is not possible to hand out a non-virtual iterator to code which is not backend-specific. An alternative
   * implementation would convert the map first into std::map<RegisterPath, RegisterInfo>, when the catalogue is
   * obtained by the applcation, which is also having an impact on performance (especially when dealing with large
   * catalogues and reading only few RegisterInfos from it).
   */
  template<typename BackendRegisterInfo>
  class const_BackendRegisterCatalogueImplIterator : public const_RegisterCatalogueImplIterator {
   public:
    explicit const_BackendRegisterCatalogueImplIterator(typename std::vector<BackendRegisterInfo*>::const_iterator it);

    void increment() override;

    void decrement() override;

    const BackendRegisterInfoBase* get() override;

    [[nodiscard]] bool isEqual(
        const std::unique_ptr<const_RegisterCatalogueImplIterator>& rightHandSide) const override;

    [[nodiscard]] std::unique_ptr<const_RegisterCatalogueImplIterator> clone() const override;

    const_BackendRegisterCatalogueImplIterator& operator++();

    const_BackendRegisterCatalogueImplIterator operator++(int);

    const_BackendRegisterCatalogueImplIterator& operator--();

    const_BackendRegisterCatalogueImplIterator operator--(int);

    const BackendRegisterInfo& operator*() const;

    const BackendRegisterInfo* operator->() const;

    explicit operator RegisterCatalogue::const_iterator() const;

    bool operator==(const const_BackendRegisterCatalogueImplIterator& rightHandSide) const;

    bool operator!=(const const_BackendRegisterCatalogueImplIterator& rightHandSide) const;

   protected:
    typename std::vector<BackendRegisterInfo*>::const_iterator theIterator;
  };

  /********************************************************************************************************************/

  /** Non-const iterator for iterating through the registers in the catalogue, used by backend implementations only */
  template<typename BackendRegisterInfo>
  class BackendRegisterCatalogueImplIterator {
   public:
    BackendRegisterCatalogueImplIterator() = default;

    explicit BackendRegisterCatalogueImplIterator(typename std::vector<BackendRegisterInfo*>::iterator theIterator_);

    BackendRegisterCatalogueImplIterator& operator++();

    BackendRegisterCatalogueImplIterator operator++(int);

    BackendRegisterCatalogueImplIterator& operator--();

    BackendRegisterCatalogueImplIterator operator--(int);

    BackendRegisterInfo& operator*() const;

    BackendRegisterInfo* operator->() const;

    explicit operator RegisterCatalogue::const_iterator() const;

    bool operator==(const BackendRegisterCatalogueImplIterator& rightHandSide) const;

    bool operator!=(const BackendRegisterCatalogueImplIterator& rightHandSide) const;

   protected:
    typename std::vector<BackendRegisterInfo*>::iterator theIterator;
  };

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  /********************************************************************************************************************/
  /* Register catalogue container implementations ********************************************************************/
  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  RegisterInfo BackendRegisterCatalogue<BackendRegisterInfo>::getRegister(const RegisterPath& name) const {
    return RegisterInfo(std::make_unique<BackendRegisterInfo>(getBackendRegister(name)));
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  BackendRegisterInfo BackendRegisterCatalogue<BackendRegisterInfo>::getBackendRegister(
      const RegisterPath& name) const {
    try {
      return catalogue.at(name);
    }
    catch(std::out_of_range&) {
      throw ChimeraTK::logic_error("BackendRegisterCatalogue::getRegister(): Register '" + name + "' does not exist.");
    }
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  bool BackendRegisterCatalogue<BackendRegisterInfo>::hasRegister(const RegisterPath& registerPathName) const {
    return catalogue.find(registerPathName) != catalogue.end();
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  size_t BackendRegisterCatalogue<BackendRegisterInfo>::getNumberOfRegisters() const {
    return catalogue.size();
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  std::unique_ptr<const_RegisterCatalogueImplIterator> BackendRegisterCatalogue<
      BackendRegisterInfo>::getConstIteratorBegin() const {
    auto it = std::make_unique<const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>>(
        insertionOrderedCatalogue.cbegin());
    return it;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  std::unique_ptr<const_RegisterCatalogueImplIterator> BackendRegisterCatalogue<
      BackendRegisterInfo>::getConstIteratorEnd() const {
    auto it = std::make_unique<const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>>(
        insertionOrderedCatalogue.cend());
    return it;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  std::unique_ptr<BackendRegisterCatalogueBase> BackendRegisterCatalogue<BackendRegisterInfo>::clone() const {
    // FIXME: Change BackendRegisterCatalogue to CRTP, i.e. it has the DERRIVED class as template parameter.
    // Like this the correct catalogue type is already created here and inherriting backends can call this clone,
    // then cast to the actual type and fill the rest of it's properties. Would get rid of the "fillFromThis()"
    // workaround and safe clone() implementations in case no data members are added.

    // Create a new instance of a BackendRegisterCatalogue with the correct type.
    // Inheriting backends will create the derrived type here.
    std::unique_ptr<BackendRegisterCatalogueBase> c = std::make_unique<BackendRegisterCatalogue<BackendRegisterInfo>>();
    // Fill the contents of the BackendRegisterCatalogue base class into the target c. This is accessing the
    // private variables and ensures consistency.
    fillFromThis(dynamic_cast<BackendRegisterCatalogue<BackendRegisterInfo>*>(c.get()));
    // Derrived backends will copy/clone their additional data members here, before
    // returning the unique_ptr. The compiler will return it via copy elision.
    return c;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  void BackendRegisterCatalogue<BackendRegisterInfo>::fillFromThis(
      BackendRegisterCatalogue<BackendRegisterInfo>* target) const {
    // FIXME: change this to a single loop which is just filling the new catalogue through the public API
    for(auto& p : catalogue) {
      target->catalogue[p.first] = getBackendRegister(p.first);
    }
    for(auto& ptr : insertionOrderedCatalogue) {
      target->insertionOrderedCatalogue.push_back(&target->catalogue[ptr->getRegisterName()]);
    }
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  void BackendRegisterCatalogue<BackendRegisterInfo>::addRegister(const BackendRegisterInfo& registerInfo) {
    if(hasRegister(registerInfo.getRegisterName())) {
      throw ChimeraTK::logic_error("BackendRegisterCatalogue::addRegister(): Register with the name " +
          registerInfo.getRegisterName() + " already exists!");
    }
    catalogue[registerInfo.getRegisterName()] = registerInfo;
    insertionOrderedCatalogue.push_back(&catalogue[registerInfo.getRegisterName()]);
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  void BackendRegisterCatalogue<BackendRegisterInfo>::removeRegister(const RegisterPath& name) {
    // check existence
    if(!hasRegister(name)) {
      throw ChimeraTK::logic_error(
          "BackendRegisterCatalogue::removeRegister(): Register '" + name + "' does not exist.");
    }

    // remove from insertion-ordered vector
    auto it = std::find_if(insertionOrderedCatalogue.begin(), insertionOrderedCatalogue.end(),
        [&](auto reg) { return reg->getRegisterName() == name; });
    assert(it != insertionOrderedCatalogue.end());
    insertionOrderedCatalogue.erase(it);

    // remove from catalogue map
    auto removed = catalogue.erase(name);
    assert(removed == 1);
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  void BackendRegisterCatalogue<BackendRegisterInfo>::modifyRegister(const BackendRegisterInfo& registerInfo) {
    if(!hasRegister(registerInfo.getRegisterName())) {
      throw ChimeraTK::logic_error("BackendRegisterCatalogue::modifyRegister(): Register '" +
          registerInfo.getRegisterName() + "' cannot be modified because it does not exist!");
    }
    catalogue[registerInfo.getRegisterName()] = registerInfo;
    // We don't have to touch the insertionOrderedCatalogue because is stores references, and this has not changed.
  }

  /********************************************************************************************************************/
  /* Iterator implementations ****************************************************************************************/
  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>::const_BackendRegisterCatalogueImplIterator(
      typename std::vector<BackendRegisterInfo*>::const_iterator it)
  : theIterator(it) {}

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  void const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>::increment() {
    ++theIterator;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  void const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>::decrement() {
    --theIterator;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  const BackendRegisterInfoBase* const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>::get() {
    return *theIterator;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  bool const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>::isEqual(
      const std::unique_ptr<const_RegisterCatalogueImplIterator>& rightHandSide) const {
    auto rhs_casted = dynamic_cast<const_BackendRegisterCatalogueImplIterator*>(rightHandSide.get());
    return rhs_casted && rhs_casted->theIterator == theIterator;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  std::unique_ptr<const_RegisterCatalogueImplIterator> const_BackendRegisterCatalogueImplIterator<
      BackendRegisterInfo>::clone() const {
    return {std::make_unique<const_BackendRegisterCatalogueImplIterator>(*this)};
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>& const_BackendRegisterCatalogueImplIterator<
      BackendRegisterInfo>::operator++() {
    ++theIterator;
    return *this;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo> const_BackendRegisterCatalogueImplIterator<
      BackendRegisterInfo>::operator++(int) {
    BackendRegisterCatalogueImplIterator<BackendRegisterInfo> temp(*this);
    ++theIterator;
    return temp;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>& const_BackendRegisterCatalogueImplIterator<
      BackendRegisterInfo>::operator--() {
    --theIterator;
    return *this;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo> const_BackendRegisterCatalogueImplIterator<
      BackendRegisterInfo>::operator--(int) {
    BackendRegisterCatalogueImplIterator<BackendRegisterInfo> temp(*this);
    --theIterator;
    return temp;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  const BackendRegisterInfo& const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>::operator*() const {
    return *(*theIterator);
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  const BackendRegisterInfo* const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>::operator->() const {
    return *theIterator;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  bool const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>::operator==(
      const const_BackendRegisterCatalogueImplIterator& rightHandSide) const {
    return rightHandSide.theIterator == theIterator;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  bool const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>::operator!=(
      const const_BackendRegisterCatalogueImplIterator& rightHandSide) const {
    return rightHandSide.theIterator != theIterator;
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  BackendRegisterCatalogueImplIterator<BackendRegisterInfo>::BackendRegisterCatalogueImplIterator(
      typename std::vector<BackendRegisterInfo*>::iterator theIterator_)
  : theIterator(theIterator_) {}

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  BackendRegisterCatalogueImplIterator<BackendRegisterInfo>& BackendRegisterCatalogueImplIterator<
      BackendRegisterInfo>::operator++() {
    ++theIterator;
    return *this;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  BackendRegisterCatalogueImplIterator<BackendRegisterInfo> BackendRegisterCatalogueImplIterator<
      BackendRegisterInfo>::operator++(int) {
    BackendRegisterCatalogueImplIterator<BackendRegisterInfo> temp(*this);
    ++theIterator;
    return temp;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  BackendRegisterCatalogueImplIterator<BackendRegisterInfo>& BackendRegisterCatalogueImplIterator<
      BackendRegisterInfo>::operator--() {
    --theIterator;
    return *this;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  BackendRegisterCatalogueImplIterator<BackendRegisterInfo> BackendRegisterCatalogueImplIterator<
      BackendRegisterInfo>::operator--(int) {
    BackendRegisterCatalogueImplIterator<BackendRegisterInfo> temp(*this);
    --theIterator;
    return temp;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  BackendRegisterInfo& BackendRegisterCatalogueImplIterator<BackendRegisterInfo>::operator*() const {
    return *(*theIterator);
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  BackendRegisterInfo* BackendRegisterCatalogueImplIterator<BackendRegisterInfo>::operator->() const {
    return *theIterator;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  BackendRegisterCatalogueImplIterator<BackendRegisterInfo>::operator RegisterCatalogue::const_iterator() const {
    return RegisterCatalogue::const_iterator{
        boost::make_shared<const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>>(theIterator)};
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  bool BackendRegisterCatalogueImplIterator<BackendRegisterInfo>::operator==(
      const BackendRegisterCatalogueImplIterator& rightHandSide) const {
    return rightHandSide.theIterator == theIterator;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  bool BackendRegisterCatalogueImplIterator<BackendRegisterInfo>::operator!=(
      const BackendRegisterCatalogueImplIterator& rightHandSide) const {
    return rightHandSide.theIterator != theIterator;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
