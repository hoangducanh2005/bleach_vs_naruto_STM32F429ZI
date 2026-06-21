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
#include "ILI9341_GFX.h"
#include "lcd_port.h"
#include "mainmenu.h"

#define UI_COLOR_BLACK        0x0000U
#define UI_COLOR_WHITE        0xFFFFU
#define UI_COLOR_PANEL        0x1082U
#define UI_COLOR_PANEL_DARK   0x0008U
#define UI_COLOR_ORANGE       0xFD20U
#define UI_COLOR_YELLOW       0xFFE0U
#define UI_COLOR_CYAN         0x07FFU
#define UI_COLOR_RED          0xF800U
#define UI_COLOR_GRAY         0x8410U

static void GameUI_DrawMenuButton(uint16_t x,
                                  uint16_t y,
                                  uint16_t width,
                                  const char *label,
                                  uint8_t selected);

void GameUI_DrawSplash(void)
{
  LCD_Port_DrawRGB565Bytes(0U,
                           0U,
                           BLEACH_VS_NARUTO_SPLASH_WIDTH,
                           BLEACH_VS_NARUTO_SPLASH_HEIGHT,
                           bleach_vs_naruto_splash_map);
}

void GameUI_DrawMainMenuBackground(void)
{
  LCD_Port_DrawRGB565Bytes(0U,
                           0U,
                           MAINMENU_WIDTH,
                           MAINMENU_HEIGHT,
                           mainmenu_map);
}

void GameUI_DrawMainMenu(void)
{
  GameUI_DrawMainMenuBackground();

  ILI9341_DrawFilledRectangleCoord(18U, 16U, 302U, 61U, UI_COLOR_PANEL_DARK);
  ILI9341_DrawHollowRectangleCoord(18U, 16U, 302U, 61U, UI_COLOR_ORANGE);
  ILI9341_DrawText("NARUTO VS BLEACH", FONT4, 59U, 24U, UI_COLOR_YELLOW, UI_COLOR_PANEL_DARK);
  ILI9341_DrawText("STM32F429I FIGHTING GAME", FONT2, 72U, 47U, UI_COLOR_WHITE, UI_COLOR_PANEL_DARK);

  ILI9341_DrawFilledRectangleCoord(30U, 78U, 290U, 205U, UI_COLOR_PANEL);
  ILI9341_DrawHollowRectangleCoord(30U, 78U, 290U, 205U, UI_COLOR_CYAN);

  GameUI_DrawMenuButton(55U, 92U, 210U, "START GAME", 1U);
  GameUI_DrawMenuButton(55U, 128U, 210U, "DIFFICULTY: NORMAL", 0U);
  GameUI_DrawMenuButton(55U, 164U, 210U, "CHARACTER: ICHIGO", 0U);

  ILI9341_DrawText("P1 ICHIGO  VS  CPU NARUTO", FONT2, 63U, 211U, UI_COLOR_WHITE, UI_COLOR_BLACK);
  ILI9341_DrawText("JOYSTICK: MOVE   BTN: SELECT", FONT1, 57U, 226U, UI_COLOR_GRAY, UI_COLOR_BLACK);
}

static void GameUI_DrawMenuButton(uint16_t x,
                                  uint16_t y,
                                  uint16_t width,
                                  const char *label,
                                  uint8_t selected)
{
  uint16_t borderColor = selected ? UI_COLOR_YELLOW : UI_COLOR_WHITE;
  uint16_t textColor = selected ? UI_COLOR_BLACK : UI_COLOR_WHITE;
  uint16_t fillColor = selected ? UI_COLOR_ORANGE : UI_COLOR_PANEL_DARK;

  ILI9341_DrawFilledRectangleCoord(x, y, x + width, y + 25U, fillColor);
  ILI9341_DrawHollowRectangleCoord(x, y, x + width, y + 25U, borderColor);

  if (selected != 0U)
  {
    ILI9341_DrawText(">", FONT3, x + 12U, y + 5U, UI_COLOR_BLACK, fillColor);
  }

  ILI9341_DrawText(label, FONT3, x + 34U, y + 5U, textColor, fillColor);
}
