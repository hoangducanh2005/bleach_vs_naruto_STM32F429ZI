#ifndef GAMEOVER_H
#define GAMEOVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define GAMEOVER_WIDTH   160U
#define GAMEOVER_HEIGHT  120U
#define GAMEOVER_SIZE    38400U

extern const uint8_t gameover_map[GAMEOVER_SIZE];

void GameOverDemo_Init(void);
void GameOverDemo_Update(void);

#ifdef __cplusplus
}
#endif

#endif /* GAMEOVER_H */
