#pragma once

class Win32Exception : public std::runtime_error {
  using Parent = std::runtime_error;

public:
  Win32Exception(const std::string &message) : Parent{message} {}
};

void throw_if_error();
template <typename T> T throw_if_error(T ret_val) {
  throw_if_error();
  return ret_val;
}
