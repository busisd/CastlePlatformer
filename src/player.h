#ifndef CASTLE_PLATFORMER_PLAYER
#define CASTLE_PLATFORMER_PLAYER

#include "util.h"

// x(0.0f), y(-100.0f), width(46.0f), height(94.0f)

class Player
{
public:
  Player() : rect{0.0, -100.0, 46.0, 94.0}, maxFallSpeed(-15) {}
  util::Rect rect;
  bool isGrounded;
  double yAccel;
  double maxFallSpeed;
};

#endif
