// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2019 Sergey Semushin <Predelnik@gmail.com>
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#pragma once

template <typename EnumT>
class enum_range_t;

template <typename EnumT>
class enum_range_iterator {
	using iterator_category = std::forward_iterator_tag;
	using value_type = EnumT;
	using difference_type = EnumT;
	using pointer = EnumT*;
	using reference = EnumT;
	EnumT m_value;
public:
	EnumT operator *() const {
		return m_value;
	}

	enum_range_iterator<EnumT> operator++() {
		enum_range_iterator val{m_value};
		m_value = static_cast<EnumT>(static_cast<int>(m_value) + 1);
		return val;
	}

	bool operator ==(const enum_range_iterator& rhs) const {
		return m_value == rhs.m_value;
	}

	bool operator !=(const enum_range_iterator& rhs) const {
		return m_value != rhs.m_value;
	}

private:
	enum_range_iterator(EnumT value) {
		m_value = value;
	}

	friend class enum_range_t<EnumT>;
};


template <typename EnumT>
class enum_range_t {
public:
	using iterator = enum_range_iterator<EnumT>;
	using value_type = EnumT;

	enum_range_t() : m_begin{static_cast<EnumT>(0)}, m_end{EnumT::COUNT} {
	}

	enum_range_t(EnumT begin) : enum_range_t{} {
		m_begin = iterator{begin};
	}

	enum_range_t(EnumT begin, EnumT end) : m_begin{begin}, m_end{end} {
	}

	iterator begin() const {
		return m_begin;
	}

	iterator end() const {
		return m_end;
	}

private:
	iterator m_begin;
	iterator m_end;
};

template <typename EnumT>
enum_range_t<EnumT> enum_range() {
	return enum_range_t<EnumT>();
}

template <typename EnumT>
enum_range_t<EnumT> enum_range(EnumT begin) {
	return enum_range_t<EnumT>(begin);
}

template <typename EnumT>
enum_range_t<EnumT> enum_range(EnumT begin, EnumT end) {
	return enum_range_t<EnumT>(begin, end);
}

