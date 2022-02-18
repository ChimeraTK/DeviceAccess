#include "RegisterCatalogue.h"
#include "BackendRegisterCatalogue.h"

namespace ChimeraTK {

  /*******************************************************************************************************************/

  RegisterInfo RegisterCatalogue::getRegister(const RegisterPath& registerPathName) const {
    return impl->getRegister(registerPathName);
  }

  /*******************************************************************************************************************/

  bool RegisterCatalogue::hasRegister(const RegisterPath& registerPathName) const {
    return impl->hasRegister(registerPathName);
  }

  /*******************************************************************************************************************/

  size_t RegisterCatalogue::getNumberOfRegisters() const { return impl->getNumberOfRegisters(); }

  /*******************************************************************************************************************/

  RegisterCatalogue::const_iterator RegisterCatalogue::cbegin() const {
    return RegisterCatalogue::const_iterator(impl->getConstIteratorBegin());
  }

  /*******************************************************************************************************************/

  RegisterCatalogue::const_iterator RegisterCatalogue::cend() const {
    return RegisterCatalogue::const_iterator(impl->getConstIteratorEnd());
  }

  /*******************************************************************************************************************/

  RegisterCatalogue::const_iterator::const_iterator(std::unique_ptr<const_RegisterCatalogueImplIterator> it)
  : impl(std::move(it)) {}

  /*******************************************************************************************************************/

  RegisterCatalogue::const_iterator& RegisterCatalogue::const_iterator::operator++() {
    impl->increment();
    return *this;
  }

  /*******************************************************************************************************************/

  RegisterCatalogue::const_iterator RegisterCatalogue::const_iterator::operator++(int) {
    RegisterCatalogue::const_iterator temp(impl->clone());
    impl->increment();
    return temp;
  }

  /*******************************************************************************************************************/

  RegisterCatalogue::const_iterator& RegisterCatalogue::const_iterator::operator--() {
    impl->decrement();
    return *this;
  }

  /*******************************************************************************************************************/

  RegisterCatalogue::const_iterator RegisterCatalogue::const_iterator::operator--(int) {
    RegisterCatalogue::const_iterator temp(impl->clone());
    impl->decrement();
    return temp;
  }

  /*******************************************************************************************************************/

  const RegisterInfoImpl* RegisterCatalogue::const_iterator::operator*() { return impl->get(); }

  /*******************************************************************************************************************/

  const RegisterInfoImpl* RegisterCatalogue::const_iterator::operator->() { return impl->get(); }

  /*******************************************************************************************************************/

  bool RegisterCatalogue::const_iterator::operator==(const RegisterCatalogue::const_iterator& rightHandSide) const {
    return impl->isEqual(rightHandSide.impl);
  }

  /*******************************************************************************************************************/

  bool RegisterCatalogue::const_iterator::operator!=(const RegisterCatalogue::const_iterator& rightHandSide) const {
    return !impl->isEqual(rightHandSide.impl);
  }

  /*******************************************************************************************************************/

} /* namespace ChimeraTK */
