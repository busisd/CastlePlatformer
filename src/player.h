#ifndef CASTLE_PLATFORMER_PLAYER
#define CASTLE_PLATFORMER_PLAYER

#include "util.h"

class Player
{
public:
  Player() : rects{game_rect : {0.0, -100.0, 46.0, 94.0}}, max_fall_speed(-15) {}
  util::Drawable rects;
  bool is_grounded;
  double y_velocity;
  double max_fall_speed;
};

#endif
