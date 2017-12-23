#pragma once

class MappedWstring {
public:
    long to_original_index(long cur_index) const { return !mapping.empty() ? mapping[cur_index] : cur_index; }
    long from_original_index(long cur_index) const { return !mapping.empty() ?
        static_cast<long> (std::lower_bound(mapping.begin (), mapping.end (), cur_index) - mapping.begin ()) : cur_index; }
public:
    std::wstring str;
    std::vector<long> mapping; // should have size str.length () or empty (if empty mapping is identity a<->a)
    // indices should correspond to offsets string `str` had in original encoding
};
