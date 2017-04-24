#ifndef ENUM_VECTOR_H
#define ENUM_VECTOR_H

// fixed size container allocated on the heap, indexed by enums
#include <vector>

template<typename EnumT, typename valueT>
class enum_vector : private std::vector < valueT >
{
  using parent_t = std::vector < valueT > ;

  static_assert (std::is_enum<EnumT>::value, "Only enum types are allowed");

public:
  explicit enum_vector (size_t size = static_cast<size_t> (EnumT::COUNT)) : parent_t (size) {}

  valueT &operator[] (EnumT key)                 { return parent_t::operator[] (static_cast<size_t> (key)); }
  const valueT &operator[] (EnumT key) const     { return parent_t::operator[] (static_cast<size_t> (key)); }
  valueT &at (EnumT key)                         { return parent_t::at (static_cast<size_t> (key)); }
  const valueT &at (EnumT key) const             { return parent_t::at (static_cast<size_t> (key)); }

  using parent_t::size;
  using parent_t::begin;
  using parent_t::end;
  using parent_t::rbegin;
  using parent_t::rend;
  using parent_t::cbegin;
  using parent_t::cend;
  using parent_t::crbegin;
  using parent_t::crend;
  using parent_t::front;
  using parent_t::back;
  using parent_t::data;
  using parent_t::empty;
  using parent_t::max_size;

  using typename parent_t::iterator;
  using typename parent_t::const_iterator;
  using typename parent_t::reverse_iterator;
  using typename parent_t::const_reverse_iterator;

  void swap (enum_vector &other) { parent_t::swap (other); }
  friend bool operator== (const enum_vector &lhs, const enum_vector &rhs) { return operator== (static_cast<const parent_t&> (lhs), static_cast<const parent_t&> (rhs)); }
  friend bool operator!= (const enum_vector &lhs, const enum_vector &rhs) { return operator!= (static_cast<const parent_t&> (lhs), static_cast<const parent_t&> (rhs)); }
  friend bool operator<  (const enum_vector &lhs, const enum_vector &rhs) { return operator<  (static_cast<const parent_t&> (lhs), static_cast<const parent_t&> (rhs)); }
  friend bool operator<= (const enum_vector &lhs, const enum_vector &rhs) { return operator<= (static_cast<const parent_t&> (lhs), static_cast<const parent_t&> (rhs)); }
  friend bool operator>  (const enum_vector &lhs, const enum_vector &rhs) { return operator>  (static_cast<const parent_t&> (lhs), static_cast<const parent_t&> (rhs)); }
  friend bool operator>= (const enum_vector &lhs, const enum_vector &rhs) { return operator>= (static_cast<const parent_t&> (lhs), static_cast<const parent_t&> (rhs)); }

  friend void swap (enum_vector &lhs, enum_vector &rhs) { std::swap (static_cast<parent_t &> (lhs), static_cast<parent_t &> (rhs)); }
};
#endif // ENUM_VECTOR_H

