#pragma once

#include "RegisterCatalogue.h"
#include <boost/make_shared.hpp>

namespace ChimeraTK {

  /*******************************************************************************************************************/
  /* Iterator class definitions **************************************************************************************/
  /*******************************************************************************************************************/

  /**
   * Virtual base class for the catalogue const iterator. The typical interator interface is realised in the
   * RegisterCatalogue::const_iterator class, which holds a pointer to this class (pimpl pattern).
   */
  class const_RegisterCatalogueImplIterator {
   public:
    virtual ~const_RegisterCatalogueImplIterator() = default;

    virtual void increment() = 0;

    virtual void decrement() = 0;

    [[nodiscard]] virtual const RegisterInfoImpl* get() = 0;

    [[nodiscard]] virtual bool isEqual(
        const std::unique_ptr<const_RegisterCatalogueImplIterator>& rightHandSide) const = 0;

    /**
     * Create copy of the iterator. This is required to implement the post-increment/decrement operators and
     * proper copy/assignment sematics of the RegisterCatalogue::const_iterator.
     */
    [[nodiscard]] virtual std::unique_ptr<const_RegisterCatalogueImplIterator> clone() const = 0;
  };

  /*******************************************************************************************************************/

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
    explicit const_BackendRegisterCatalogueImplIterator(
        typename std::map<RegisterPath, BackendRegisterInfo>::const_iterator it);

    void increment() override;

    void decrement() override;

    const RegisterInfoImpl* get() override;

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
    typename std::map<RegisterPath, BackendRegisterInfo>::const_iterator theIterator;
  };

  /*******************************************************************************************************************/

  /** Non-const iterator for iterating through the registers in the catalogue, used by backend implementations only */
  template<typename BackendRegisterInfo>
  class BackendRegisterCatalogueImplIterator {
   public:
    BackendRegisterCatalogueImplIterator() = default;

    explicit BackendRegisterCatalogueImplIterator(
        typename std::map<RegisterPath, BackendRegisterInfo>::iterator theIterator_);

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
    typename std::map<RegisterPath, BackendRegisterInfo>::iterator theIterator;
  };

  /*******************************************************************************************************************/
  /* Register catalogue container class definitions ******************************************************************/
  /*******************************************************************************************************************/

  /** Pure virtual implementation base class for the register catalogue. */
  class RegisterCatalogueImpl {
   public:
    RegisterCatalogueImpl() = default;

    RegisterCatalogueImpl(const RegisterCatalogueImpl& other);
    RegisterCatalogueImpl(RegisterCatalogueImpl&& other) = default;

    RegisterCatalogueImpl& operator=(const RegisterCatalogueImpl& other);
    RegisterCatalogueImpl& operator=(RegisterCatalogueImpl&& other) = default;

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
    [[nodiscard]] virtual std::unique_ptr<RegisterCatalogueImpl> clone() const = 0;
  };

  /*******************************************************************************************************************/

  /**
   * Interface for backends to the register catalogue. In addition to the functionality offered by the RegisterCatalogue
   * class, the content of the catalogue can be modified through this interface.
   * 
   * Backend implementations should instantiate this class with their backend-specific implementation of the
   * RegisterInfoImpl class.
   */
  template<typename BackendRegisterInfo>
  class BackendRegisterCatalogue : RegisterCatalogueImpl {
   public:
    [[nodiscard]] RegisterInfo getRegister(const RegisterPath& registerPathName) const override;

    [[nodiscard]] BackendRegisterInfo& getBackendRegister(const RegisterPath& registerPathName);

    [[nodiscard]] const BackendRegisterInfo& getBackendRegister(const RegisterPath& registerPathName) const;

    [[nodiscard]] bool hasRegister(const RegisterPath& registerPathName) const override;

    [[nodiscard]] size_t getNumberOfRegisters() const override;

    [[nodiscard]] std::unique_ptr<const_RegisterCatalogueImplIterator> getConstIteratorBegin() const override;

    [[nodiscard]] std::unique_ptr<const_RegisterCatalogueImplIterator> getConstIteratorEnd() const override;

    [[nodiscard]] std::unique_ptr<RegisterCatalogueImpl> clone() const override;

    /**
     * Add register information to the catalogue. The full path name of the register is taken from the RegisterInfo
     * structure.
     */
    void addRegister(BackendRegisterInfo registerInfo);

    /**
     * Remove register as identified by the given name from the catalogue. Throws ChimeraTK::logic_error if register
     * does not exist in the catalogue.
     */
    void removeRegister(const RegisterPath& name);

    /** Return begin iterator for iterating through the registers in the catalogue */
    [[nodiscard]] BackendRegisterCatalogueImplIterator<BackendRegisterInfo> begin() {
      return BackendRegisterCatalogueImplIterator<BackendRegisterInfo>{catalogue.begin()};
    }

    /** Return end iterator for iterating through the registers in the catalogue */
    [[nodiscard]] BackendRegisterCatalogueImplIterator<BackendRegisterInfo> end() {
      return BackendRegisterCatalogueImplIterator<BackendRegisterInfo>{catalogue.end()};
    }

    /** Return const begin iterators for iterating through the registers in the catalogue */
    [[nodiscard]] const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo> cbegin() const {
      return const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>{catalogue.begin()};
    }

    /** Return const end iterators for iterating through the registers in the catalogue */
    [[nodiscard]] const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo> cend() const {
      return const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>{catalogue.end()};
    }

   protected:
    std::map<RegisterPath, BackendRegisterInfo> catalogue;
  };

  /*******************************************************************************************************************/
  /*******************************************************************************************************************/

  /*******************************************************************************************************************/
  /* Iterator implementations ****************************************************************************************/
  /*******************************************************************************************************************/

  template<typename BackendRegisterInfo>
  const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>::const_BackendRegisterCatalogueImplIterator(
      typename std::map<RegisterPath, BackendRegisterInfo>::const_iterator it)
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
  const RegisterInfoImpl* const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>::get() {
    return &(theIterator->second);
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
    auto* p = new const_BackendRegisterCatalogueImplIterator(*this);
    return std::unique_ptr<const_RegisterCatalogueImplIterator>(p);
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
    return *(theIterator->second);
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  const BackendRegisterInfo* const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>::operator->() const {
    return theIterator->second;
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
      typename std::map<RegisterPath, BackendRegisterInfo>::iterator theIterator_)
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
    return theIterator->second;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  BackendRegisterInfo* BackendRegisterCatalogueImplIterator<BackendRegisterInfo>::operator->() const {
    return &(theIterator->second);
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

  /*******************************************************************************************************************/
  /* Register catalogue container implementations ********************************************************************/
  /*******************************************************************************************************************/

  template<typename BackendRegisterInfo>
  RegisterInfo BackendRegisterCatalogue<BackendRegisterInfo>::getRegister(const RegisterPath& name) const {
    try {
      return RegisterInfo(std::make_unique<BackendRegisterInfo>(catalogue.at(name)));
    }
    catch(std::out_of_range&) {
      throw ChimeraTK::logic_error("BackendRegisterCatalogue::getRegister(): Register '" + name + "' does not exist.");
    }
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  BackendRegisterInfo& BackendRegisterCatalogue<BackendRegisterInfo>::getBackendRegister(const RegisterPath& name) {
    try {
      return catalogue.at(name);
    }
    catch(std::out_of_range&) {
      throw ChimeraTK::logic_error("BackendRegisterCatalogue::getRegister(): Register '" + name + "' does not exist.");
    }
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  const BackendRegisterInfo& BackendRegisterCatalogue<BackendRegisterInfo>::getBackendRegister(
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
  size_t BackendRegisterCatalogue<BackendRegisterInfo>::getNumberOfRegisters() const {
    return catalogue.size();
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  std::unique_ptr<const_RegisterCatalogueImplIterator> BackendRegisterCatalogue<
      BackendRegisterInfo>::getConstIteratorBegin() const {
    auto it = std::make_unique<const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>>(catalogue.cbegin());
    return it;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  std::unique_ptr<const_RegisterCatalogueImplIterator> BackendRegisterCatalogue<
      BackendRegisterInfo>::getConstIteratorEnd() const {
    auto it = std::make_unique<const_BackendRegisterCatalogueImplIterator<BackendRegisterInfo>>(catalogue.cend());
    return it;
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  std::unique_ptr<RegisterCatalogueImpl> BackendRegisterCatalogue<BackendRegisterInfo>::clone() const {
    auto* c = new BackendRegisterCatalogue<BackendRegisterInfo>();
    for(auto& p : catalogue) {
      c->catalogue[p.first] = getBackendRegister(p.first);
    }
    return std::unique_ptr<RegisterCatalogueImpl>(c);
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  void BackendRegisterCatalogue<BackendRegisterInfo>::addRegister(BackendRegisterInfo registerInfo) {
    catalogue[registerInfo.getRegisterName()] = std::move(registerInfo);
  }

  /********************************************************************************************************************/

  template<typename BackendRegisterInfo>
  void BackendRegisterCatalogue<BackendRegisterInfo>::removeRegister(const RegisterPath& name) {
    auto removed = catalogue.erase(name);
    if(removed != 1) {
      throw ChimeraTK::logic_error(
          "BackendRegisterCatalogue::removeRegister(): Register '" + name + "' does not exist.");
    }
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
