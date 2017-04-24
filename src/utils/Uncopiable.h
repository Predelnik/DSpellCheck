#pragma once

class Uncopiable
{
protected:
  Uncopiable () {}
  ~Uncopiable () {}
  Uncopiable (const Uncopiable&) = delete;
  Uncopiable& operator=(const Uncopiable&) = delete;
};
