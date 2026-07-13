/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : naruto_full_nine_tails_moveset.h
  * @brief          : Naruto Full Nine Tails gameplay animation frames generated from moveset JSON.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __NARUTO_FULL_NINE_TAILS_MOVESET_H
#define __NARUTO_FULL_NINE_TAILS_MOVESET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
  NARUTO_FULL_NINE_TAILS_MOVE_IDLE = 0U,
  NARUTO_FULL_NINE_TAILS_MOVE_RUN = 1U,
  NARUTO_FULL_NINE_TAILS_MOVE_SLIDE = 2U,
  NARUTO_FULL_NINE_TAILS_MOVE_JUMP = 3U,
  NARUTO_FULL_NINE_TAILS_MOVE_BLOCK = 4U,
  NARUTO_FULL_NINE_TAILS_MOVE_HIT = 5U,
  NARUTO_FULL_NINE_TAILS_MOVE_ATTACK1 = 6U,
  NARUTO_FULL_NINE_TAILS_MOVE_ATTACK2 = 7U,
  NARUTO_FULL_NINE_TAILS_MOVE_ATTACK3 = 8U,
  NARUTO_FULL_NINE_TAILS_MOVE_SKILL1 = 9U,
  NARUTO_FULL_NINE_TAILS_MOVE_DEAD = 10U,
  NARUTO_FULL_NINE_TAILS_MOVE_STATE_COUNT = 11U
} NarutoFullNineTailsMoveState;

typedef struct
{
  const uint16_t *pixels;
  uint16_t width;
  uint16_t height;
  int16_t pivotX;
  int16_t pivotY;
  uint16_t durationMs;
} NarutoFullNineTailsMoveFrame;

typedef struct
{
  const NarutoFullNineTailsMoveFrame *frames;
  uint8_t frameCount;
  uint8_t loop;
  uint8_t holdLast;
  const uint16_t * const *casterEffectFrames;
  uint8_t casterEffectFrameCount;
  uint8_t casterEffectStartFrameIdx;
  uint8_t casterEffectLoop;
  uint16_t casterEffectWidth;
  uint16_t casterEffectHeight;
  const uint16_t * const *projectileFrames;
  uint8_t projectileFrameCount;
  uint8_t projectileLoop;
  uint16_t projectileWidth;
  uint16_t projectileHeight;
} NarutoFullNineTailsMoveAnimation;

extern const NarutoFullNineTailsMoveAnimation naruto_full_nine_tails_move_animations[NARUTO_FULL_NINE_TAILS_MOVE_STATE_COUNT];

#ifdef __cplusplus
}
#endif

#endif /* __NARUTO_FULL_NINE_TAILS_MOVESET_H */
