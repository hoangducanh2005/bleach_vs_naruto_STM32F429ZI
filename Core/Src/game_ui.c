/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : game_ui.c
  * @brief          : Simple UI drawing helpers for the Naruto vs Bleach game.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "game_ui.h"

#include "bleach_vs_naruto_splash.h"
#include "lcd_port.h"

void GameUI_DrawSplash(void)
{
  LCD_Port_DrawRGB565Bytes(0U,
                           0U,
                           BLEACH_VS_NARUTO_SPLASH_WIDTH,
                           BLEACH_VS_NARUTO_SPLASH_HEIGHT,
                           bleach_vs_naruto_splash_map);
}
