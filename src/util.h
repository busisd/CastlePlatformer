#ifndef CASTLE_PLATFORMER_UTIL
#define CASTLE_PLATFORMER_UTIL

#include <unordered_set>

namespace util
{
  template <typename T>
  void PrintSet(std::unordered_set<T> const &s);

  template <typename T>
  bool Contains(std::unordered_set<T> const &s, T t);

  struct Rect
  {
    double x;
    double y;
    double w;
    double h;
  };

  struct Point
  {
    double x;
    double y;
  };

  bool Collides(Rect rect1, Rect rect2);

  template <typename T, typename... Args>
  void prettyLog(T t, Args... args);
}

#include "util.tcc"

#endif
