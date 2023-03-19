#ifndef CASTLE_PLATFORMER_UTIL
#define CASTLE_PLATFORMER_UTIL

#include <unordered_set>

namespace util
{
  template <typename T>
  void PrintSet(std::unordered_set<T> const &s);

  template <typename T>
  bool Contains(std::unordered_set<T> const &s, T t);
}

#include "util.tcc"

#endif
