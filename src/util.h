#ifndef CASTLE_PLATFORMER_UTIL
#define CASTLE_PLATFORMER_UTIL

#include <unordered_set>
#include <SDL.h>

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

  struct SizedTexture
  {
    int w;
    int h;
    SDL_Texture *sdl_texture;
  };

  bool Collides(Rect rect1, Rect rect2);

  template <typename T, typename... Args>
  void prettyLog(T t, Args... args);

  int PositiveModulo(int i, int n);

  class Drawable
  {
  public:
    util::Rect game_rect;

    SDL_Rect draw_rect;
    SDL_RendererFlip flip = SDL_FLIP_NONE;
    util::SizedTexture texture;
    bool is_repeating_texture = false;
  };
}

#include "util.tcc"

#endif
