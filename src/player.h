#ifndef CASTLE_PLATFORMER_PLAYER
#define CASTLE_PLATFORMER_PLAYER

class Player
{
public:
  Player() : x(0), y(-100), width(46), height(94) {}
  float x;
  float y;
  float width;
  float height;
  bool isGrounded;
  float yAccel;
};

#endif
