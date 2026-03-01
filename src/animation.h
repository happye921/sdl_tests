#pragma once
#include "timer.h"

class Animation
{
  Timer timer;
  int frameCount;

public:
  Animation() : timer(0), frameCount(0)
  {
  }
  Animation(int frameCount, float length) : timer(length), frameCount(frameCount)
  {
  }

  float getLength() const
  {
    return timer.getLength();
  }

  int currentFrame() const
  {
    return static_cast<int>(timer.getTime() / timer.getLength() * frameCount);
  }

  void step(float deltaTime)
  {
    timer.step(deltaTime);
  }
};
