#include "combat_box.h"

CombatBox CombatBox_ToWorld(CombatBox local,
                            int16_t actorX,
                            int16_t actorY,
                            int8_t facing)
{
  CombatBox world = local;

  if (facing < 0)
  {
    world.x = (int16_t)(actorX - local.x - local.w);
  }
  else
  {
    world.x = (int16_t)(actorX + local.x);
  }

  world.y = (int16_t)(actorY + local.y);
  return world;
}

uint8_t CombatBox_Overlap(CombatBox a, CombatBox b)
{
  if ((a.w <= 0) || (a.h <= 0) || (b.w <= 0) || (b.h <= 0))
  {
    return 0U;
  }

  return ((a.x < (int16_t)(b.x + b.w)) &&
          ((int16_t)(a.x + a.w) > b.x) &&
          (a.y < (int16_t)(b.y + b.h)) &&
          ((int16_t)(a.y + a.h) > b.y))
             ? 1U
             : 0U;
}

int16_t CombatBox_ClampI16(int16_t value, int16_t minValue, int16_t maxValue)
{
  if (value < minValue)
  {
    return minValue;
  }

  if (value > maxValue)
  {
    return maxValue;
  }

  return value;
}
