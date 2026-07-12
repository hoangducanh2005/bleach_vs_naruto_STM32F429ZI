/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : game_ui.h
  * @brief          : Simple UI drawing helpers for the Naruto vs Bleach game.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __GAME_UI_H
#define __GAME_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void GameUI_DrawSplash(void);
void GameUI_DrawMainMenuBackground(void);
void GameUI_DrawMainMenu(void);
void GameUI_DrawDifficultySelect(uint8_t selectedDifficulty);
void GameUI_DrawCharacterSelect(uint8_t selectedCharacter, uint8_t cpuCharacter);
void GameUI_UpdateCharacterSelect(uint8_t previousCharacter,
                                  uint8_t selectedCharacter,
                                  uint8_t cpuCharacter);

#ifdef __cplusplus
}
#endif

#endif /* __GAME_UI_H */
