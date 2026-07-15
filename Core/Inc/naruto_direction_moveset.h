#ifndef __NARUTO_DIRECTION_MOVESET_H
#define __NARUTO_DIRECTION_MOVESET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define NARUTO_DIRECTION_FRAME_COUNT 14U

typedef struct
{
  const uint16_t *pixels;
  uint16_t width;
  uint16_t height;
  int16_t pivotX;
  int16_t pivotY;
  uint16_t durationMs;
} NarutoDirectionFrame;

extern const NarutoDirectionFrame naruto_direction_frames[NARUTO_DIRECTION_FRAME_COUNT];

#ifdef __cplusplus
}
#endif

#endif /* __NARUTO_DIRECTION_MOVESET_H */
