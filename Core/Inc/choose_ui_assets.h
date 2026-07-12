#ifndef CHOOSE_UI_ASSETS_H
#define CHOOSE_UI_ASSETS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define CHOOSE_CHARACTER_COUNT 6U
#define CHOOSE_AVATAR_WIDTH    40U
#define CHOOSE_AVATAR_HEIGHT   40U
#define CHOOSE_AVATAR_SIZE     3200U
#define CHOOSE_BANNER_WIDTH    150U
#define CHOOSE_BANNER_HEIGHT   50U
#define CHOOSE_BANNER_SIZE     15000U

typedef enum
{
  CHOOSE_CHARACTER_NARUTO = 0U,
  CHOOSE_CHARACTER_NINE_TAIL = 1U,
  CHOOSE_CHARACTER_SASUKE = 2U,
  CHOOSE_CHARACTER_ICHIGO = 3U,
  CHOOSE_CHARACTER_HOLLOW = 4U,
  CHOOSE_CHARACTER_GIN = 5U
} ChooseCharacterId;

extern const uint8_t choose_avatar_naruto_map[CHOOSE_AVATAR_SIZE];
extern const uint8_t choose_banner_naruto_map[CHOOSE_BANNER_SIZE];
extern const uint8_t choose_avatar_nine_tail_map[CHOOSE_AVATAR_SIZE];
extern const uint8_t choose_banner_nine_tail_map[CHOOSE_BANNER_SIZE];
extern const uint8_t choose_avatar_sasuke_map[CHOOSE_AVATAR_SIZE];
extern const uint8_t choose_banner_sasuke_map[CHOOSE_BANNER_SIZE];
extern const uint8_t choose_avatar_ichigo_map[CHOOSE_AVATAR_SIZE];
extern const uint8_t choose_banner_ichigo_map[CHOOSE_BANNER_SIZE];
extern const uint8_t choose_avatar_hollow_map[CHOOSE_AVATAR_SIZE];
extern const uint8_t choose_banner_hollow_map[CHOOSE_BANNER_SIZE];
extern const uint8_t choose_avatar_gin_map[CHOOSE_AVATAR_SIZE];
extern const uint8_t choose_banner_gin_map[CHOOSE_BANNER_SIZE];

extern const uint8_t * const choose_avatar_maps[CHOOSE_CHARACTER_COUNT];
extern const uint8_t * const choose_banner_maps[CHOOSE_CHARACTER_COUNT];

#ifdef __cplusplus
}
#endif

#endif /* CHOOSE_UI_ASSETS_H */
