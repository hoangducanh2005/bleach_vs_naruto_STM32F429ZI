#ifndef __COMBAT_TYPES_H
#define __COMBAT_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
  COMBAT_CHARACTER_NARUTO = 0U,
  COMBAT_CHARACTER_SASUKE,
  COMBAT_CHARACTER_ICHIGO
} CombatCharacterId;

typedef enum
{
  COMBAT_ANIM_IDLE = 0U,
  COMBAT_ANIM_RUN,
  COMBAT_ANIM_BLOCK,
  COMBAT_ANIM_JUMP,
  COMBAT_ANIM_ATTACK,
  COMBAT_ANIM_SKILL,
  COMBAT_ANIM_HIT,
  COMBAT_ANIM_DEAD
} CombatAnimState;

typedef enum
{
  COMBAT_INPUT_NONE = 0U,
  COMBAT_INPUT_LEFT = 1U << 0,
  COMBAT_INPUT_RIGHT = 1U << 1,
  COMBAT_INPUT_DOWN = 1U << 2,
  COMBAT_INPUT_UP = 1U << 3,
  COMBAT_INPUT_ATTACK = 1U << 4,
  COMBAT_INPUT_JUMP = 1U << 5,
  COMBAT_INPUT_SKILL = 1U << 6,
  COMBAT_INPUT_BLOCK = COMBAT_INPUT_DOWN
} CombatInputFlags;

typedef enum
{
  COMBAT_HIT_LEVEL_MID = 0U,
  COMBAT_HIT_LEVEL_HIGH,
  COMBAT_HIT_LEVEL_LOW,
  COMBAT_HIT_LEVEL_PROJECTILE
} CombatHitLevel;

typedef struct
{
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;
} CombatBox;

typedef struct
{
  uint8_t startFrame;
  uint8_t endFrame;
  CombatBox box;
  uint8_t damage;
  uint8_t blockDamage;
  uint16_t hitStunMs;
  uint16_t blockStunMs;
  int16_t knockbackX;
  int16_t knockbackY;
  CombatHitLevel level;
  uint8_t oneHitOnly;
} CombatHitboxDef;

typedef struct
{
  const uint16_t *pixels;
  uint16_t width;
  uint16_t height;
  int16_t pivotX;
  int16_t pivotY;
  uint16_t durationMs;
} CombatFrameView;

#ifdef __cplusplus
}
#endif

#endif /* __COMBAT_TYPES_H */
