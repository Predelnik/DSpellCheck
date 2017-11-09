#pragma once
#include <array>

// enum indexed std::array

template <typename EnumT, typename ValueT,
          size_t Count = static_cast<size_t>(EnumT::COUNT)>
// COUNT could be changed to something more suitable
class enum_array : std::array<ValueT, Count>
{
public:
    using self = enum_array;
    using parent_t = std::array<ValueT, Count>;
    using parent_t::value_type;
    using parent_t::size_type;
    using parent_t::difference_type;
    using parent_t::reference;
    using parent_t::const_reference;
    using parent_t::pointer;
    using parent_t::const_pointer;
    using parent_t::iterator;
    using parent_t::const_iterator;
    using parent_t::const_reverse_iterator;

    reference at(EnumT index)
    {
        return parent_t::at(static_cast<size_t>(index));
    }

    const_reference at(EnumT index) const
    {
        return parent_t::at(static_cast<size_t>(index));
    }

    reference operator[](EnumT index)
    {
        return parent_t::operator[](static_cast<size_t>(index));
    }

    const_reference operator[](EnumT index) const
    {
        return parent_t::operator[](static_cast<size_t>(index));
    }

    using parent_t::front;
    using parent_t::back;
    using parent_t::data;

    using parent_t::begin;
    using parent_t::end;
    using parent_t::cbegin;
    using parent_t::cend;
    using parent_t::rbegin;
    using parent_t::rend;
    using parent_t::crbegin;
    using parent_t::crend;

    using parent_t::empty;
    using parent_t::size;
    using parent_t::max_size;

    using parent_t::fill;

    void swap(self& other)
    {
        parent_t::swap(static_cast<parent_t &>(other));
    }

    friend bool operator==(const self& lhs, const self& rhs)
    {
        return static_cast<const parent_t &>(lhs) ==
            static_cast<const parent_t &>(rhs);
    }

    friend bool operator<(const self& lhs, const self& rhs)
    {
        return static_cast<const parent_t &>(lhs) <
            static_cast<const parent_t &>(rhs);
    }

    friend bool operator>(const self& lhs, const self& rhs)
    {
        return static_cast<const parent_t &>(lhs) >
            static_cast<const parent_t &>(rhs);
    }

    friend bool operator<=(const self& lhs, const self& rhs)
    {
        return static_cast<const parent_t &>(lhs) <=
            static_cast<const parent_t &>(rhs);
    }

    friend bool operator>=(const self& lhs, const self& rhs)
    {
        return static_cast<const parent_t &>(lhs) >=
            static_cast<const parent_t &>(rhs);
    }

    template <EnumT Index>
    friend ValueT& get(self& value)
    {
        using std::get;
        return get<static_cast<size_t>(Index)>(static_cast<parent_t &>(value));
    }

    template <EnumT Index>
    friend ValueT&& get(self&& value)
    {
        using std::get;
        return std::move(
            get<static_cast<size_t>(Index)>(static_cast<parent_t &&>(value)));
    }

    template <EnumT Index>
    friend const ValueT& get(const self& value)
    {
        using std::get;
        return get<static_cast<size_t>(Index)>(
            static_cast<const parent_t &>(value));
    }

    friend void swap(self& lhs, self& rhs)
    {
        using std::swap;
        return swap(static_cast<parent_t &>(lhs), static_cast<parent_t &>(rhs));
    }
};
