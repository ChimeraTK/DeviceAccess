// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "RegisterCatalogue.h"

#include "BackendRegisterCatalogue.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  RegisterCatalogue::RegisterCatalogue(std::unique_ptr<BackendRegisterCatalogueBase>&& impl) : _impl(std::move(impl)) {}

  /********************************************************************************************************************/

  RegisterCatalogue::RegisterCatalogue(const RegisterCatalogue& other) : _impl(other._impl->clone()) {}

  /********************************************************************************************************************/

  RegisterCatalogue::RegisterCatalogue(RegisterCatalogue&& other) noexcept : _impl(std::move(other._impl)) {}

  /********************************************************************************************************************/

  RegisterCatalogue& RegisterCatalogue::operator=(const RegisterCatalogue& other) {
    _impl = other._impl->clone();
    return *this;
  }

  /********************************************************************************************************************/

  RegisterCatalogue& RegisterCatalogue::operator=(RegisterCatalogue&& other) noexcept {
    _impl = std::move(other._impl);
    return *this;
  }

  /********************************************************************************************************************/

  // need to declare in .cc file, since the type RegisterCatalogueImpl must be complete
  RegisterCatalogue::~RegisterCatalogue() = default;

  /********************************************************************************************************************/

  RegisterInfo RegisterCatalogue::getRegister(const RegisterPath& registerPathName) const {
    return _impl->getRegister(registerPathName);
  }

  /********************************************************************************************************************/

  bool RegisterCatalogue::hasRegister(const RegisterPath& registerPathName) const {
    return _impl->hasRegister(registerPathName);
  }

  /********************************************************************************************************************/

  size_t RegisterCatalogue::getNumberOfRegisters() const {
    return _impl->getNumberOfRegisters();
  }

  /********************************************************************************************************************/

  RegisterCatalogue::const_iterator RegisterCatalogue::begin() const {
    return RegisterCatalogue::const_iterator(_impl->getConstIteratorBegin());
  }

  /********************************************************************************************************************/

  RegisterCatalogue::const_iterator RegisterCatalogue::end() const {
    return RegisterCatalogue::const_iterator(_impl->getConstIteratorEnd());
  }

  /********************************************************************************************************************/

  RegisterCatalogue::const_iterator::const_iterator(std::unique_ptr<const_RegisterCatalogueImplIterator> it)
  : _impl(std::move(it)) {}

  /********************************************************************************************************************/

  RegisterCatalogue::const_iterator& RegisterCatalogue::const_iterator::operator++() {
    _impl->increment();
    return *this;
  }

  /********************************************************************************************************************/

  RegisterCatalogue::const_iterator RegisterCatalogue::const_iterator::operator++(int) {
    RegisterCatalogue::const_iterator temp(_impl->clone());
    _impl->increment();
    return temp;
  }

  /********************************************************************************************************************/

  RegisterCatalogue::const_iterator& RegisterCatalogue::const_iterator::operator--() {
    _impl->decrement();
    return *this;
  }

  /********************************************************************************************************************/

  RegisterCatalogue::const_iterator RegisterCatalogue::const_iterator::operator--(int) {
    RegisterCatalogue::const_iterator temp(_impl->clone());
    _impl->decrement();
    return temp;
  }

  /********************************************************************************************************************/

  const BackendRegisterInfoBase& RegisterCatalogue::const_iterator::operator*() {
    return *(_impl->get());
  }

  /********************************************************************************************************************/

  const BackendRegisterInfoBase* RegisterCatalogue::const_iterator::operator->() {
    return _impl->get();
  }

  /********************************************************************************************************************/

  bool RegisterCatalogue::const_iterator::operator==(const RegisterCatalogue::const_iterator& rightHandSide) const {
    return _impl->isEqual(rightHandSide._impl);
  }

  /********************************************************************************************************************/

  bool RegisterCatalogue::const_iterator::operator!=(const RegisterCatalogue::const_iterator& rightHandSide) const {
    return !_impl->isEqual(rightHandSide._impl);
  }

  /********************************************************************************************************************/

  const BackendRegisterCatalogueBase& RegisterCatalogue::getImpl() const {
    return *_impl;
  }

  /********************************************************************************************************************/

  HiddenRange RegisterCatalogue::hiddenRegisters() const {
    return _impl->hiddenRegisters();
  }

  /********************************************************************************************************************/

} /* namespace ChimeraTK */
