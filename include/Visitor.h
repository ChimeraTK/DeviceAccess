#pragma once

namespace ChimeraTK {

/* Losely based on
 * https://stackoverflow.com/questions/11796121/implementing-the-visitor-pattern-using-c-templates#11802080
 */

template <typename... Types> class Visitor;

template <typename T> class Visitor<T> {
public:
  virtual void dispatch(const T &t) = 0;
};

template <typename T, typename... Types>
class Visitor<T, Types...> : public Visitor<T>, public Visitor<Types...> {
public:
  using Visitor<Types...>::dispatch;
  using Visitor<T>::dispatch;
};

} // namespace ChimeraTK
