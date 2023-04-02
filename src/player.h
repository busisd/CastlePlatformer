#ifndef CASTLE_PLATFORMER_PLAYER
#define CASTLE_PLATFORMER_PLAYER

class Player
{
public:
  Player() : x(0.0f), y(-100.0f), width(46.0f), height(94.0f) {}
  double x;
  double y;
  double width;
  double height;
  bool isGrounded;
  double yAccel;
};

#endif
