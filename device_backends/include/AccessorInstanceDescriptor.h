#pragma once

#include "RegisterPath.h"
#include "AccessMode.h"

#include <typeindex>

namespace ChimeraTK {
  /** Helper class to have a complete descriton to distinguish all accessors.
   *  It contains all the information given to DeviceBackend:getNDRegisterAccessor, incl. the offset in the
   *  register which is not known to the accessor itself.
   */
  struct AccessorInstanceDescriptor {
    RegisterPath name;
    std::type_index type;
    size_t numberOfWords;
    size_t wordOffsetInRegister;
    AccessModeFlags flags;
    AccessorInstanceDescriptor(RegisterPath name_, std::type_index type_, size_t numberOfWords_,
        size_t wordOffsetInRegister_, AccessModeFlags flags_)
    : name(name_), type(type_), numberOfWords(numberOfWords_), wordOffsetInRegister(wordOffsetInRegister_),
      flags(flags_) {}
    bool operator<(AccessorInstanceDescriptor const& other) const;
  };

} // namespace ChimeraTK
