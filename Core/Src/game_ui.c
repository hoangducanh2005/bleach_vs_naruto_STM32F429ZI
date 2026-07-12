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
#include "choose_char.h"
#include "choose_ui_assets.h"
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
#define UI_COLOR_SHADOW       0x0004U

#define CHARACTER_GRID_X0       80U
#define CHARACTER_GRID_Y0       58U
#define CHARACTER_GRID_GAP_X    20U
#define CHARACTER_GRID_GAP_Y    14U
#define CHARACTER_CURSOR_PAD    2U
#define CHARACTER_CURSOR_SIZE   44U
#define CHARACTER_BANNER_Y      184U
#define CHARACTER_P1_BANNER_X   5U
#define CHARACTER_CPU_BANNER_X  165U

static void GameUI_DrawMenuButton(uint16_t x,
                                  uint16_t y,
                                  uint16_t width,
                                  const char *label,
                                  uint8_t selected);
static const char *GameUI_GetDifficultyMenuLabel(uint8_t difficulty);
static const char *GameUI_GetCharacterMenuLabel(uint8_t character);
static void GameUI_DrawDifficultyOption(uint16_t y,
                                        const char *label,
                                        uint8_t selected);
static void GameUI_DrawChooseBackgroundRect(uint16_t x,
                                            uint16_t y,
                                            uint16_t width,
                                            uint16_t height);
static void GameUI_DrawCharacterGrid(void);
static void GameUI_DrawCharacterAvatar(uint8_t character);
static void GameUI_DrawCharacterCursor(uint8_t character);
static void GameUI_DrawCharacterBanners(uint8_t selectedCharacter,
                                        uint8_t cpuCharacter);
static uint16_t GameUI_GetCharacterAvatarX(uint8_t character);
static uint16_t GameUI_GetCharacterAvatarY(uint8_t character);

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
  GameUI_DrawMainMenuSelection(0U, 1U, CHOOSE_CHARACTER_HOLLOW);
}

void GameUI_DrawMainMenuSelection(uint8_t selectedMenu,
                                  uint8_t selectedDifficulty,
                                  uint8_t selectedCharacter)
{
  GameUI_DrawMainMenuBackground();

  ILI9341_DrawFilledRectangleCoord(18U, 16U, 302U, 61U, UI_COLOR_PANEL_DARK);
  ILI9341_DrawHollowRectangleCoord(18U, 16U, 302U, 61U, UI_COLOR_ORANGE);
  ILI9341_DrawText("NARUTO VS BLEACH", FONT4, 59U, 24U, UI_COLOR_YELLOW, UI_COLOR_PANEL_DARK);
  ILI9341_DrawText("STM32F429I FIGHTING GAME", FONT2, 72U, 47U, UI_COLOR_WHITE, UI_COLOR_PANEL_DARK);

  ILI9341_DrawFilledRectangleCoord(30U, 78U, 290U, 205U, UI_COLOR_PANEL);
  ILI9341_DrawHollowRectangleCoord(30U, 78U, 290U, 205U, UI_COLOR_CYAN);

  GameUI_DrawMenuButton(55U, 92U, 210U, "START COMBAT", selectedMenu == 0U);
  GameUI_DrawMenuButton(55U,
                        128U,
                        210U,
                        GameUI_GetDifficultyMenuLabel(selectedDifficulty),
                        selectedMenu == 1U);
  GameUI_DrawMenuButton(55U,
                        164U,
                        210U,
                        GameUI_GetCharacterMenuLabel(selectedCharacter),
                        selectedMenu == 2U);

  ILI9341_DrawText("DEMO VIZARD  VS  CPU SASUKE", FONT2, 58U, 211U, UI_COLOR_WHITE, UI_COLOR_BLACK);
  ILI9341_DrawText("JOYSTICK: MOVE  ATTACK: SELECT", FONT1, 48U, 226U, UI_COLOR_GRAY, UI_COLOR_BLACK);
}

void GameUI_DrawDifficultySelect(uint8_t selectedDifficulty)
{
  GameUI_DrawMainMenuBackground();

  ILI9341_DrawFilledRectangleCoord(22U, 18U, 298U, 58U, UI_COLOR_PANEL_DARK);
  ILI9341_DrawHollowRectangleCoord(22U, 18U, 298U, 58U, UI_COLOR_ORANGE);
  ILI9341_DrawText("SELECT DIFFICULTY", FONT4, 54U, 28U, UI_COLOR_YELLOW, UI_COLOR_PANEL_DARK);

  ILI9341_DrawFilledRectangleCoord(36U, 76U, 284U, 197U, UI_COLOR_PANEL);
  ILI9341_DrawHollowRectangleCoord(36U, 76U, 284U, 197U, UI_COLOR_CYAN);

  GameUI_DrawDifficultyOption(91U, "EASY", selectedDifficulty == 0U);
  GameUI_DrawDifficultyOption(126U, "NORMAL", selectedDifficulty == 1U);
  GameUI_DrawDifficultyOption(161U, "HARD", selectedDifficulty == 2U);

  ILI9341_DrawText("CONFIRM: BACK TO MAIN MENU", FONT1, 59U, 216U, UI_COLOR_WHITE, UI_COLOR_BLACK);
  ILI9341_DrawText("UP/DOWN: CHANGE LEVEL", FONT1, 76U, 229U, UI_COLOR_GRAY, UI_COLOR_BLACK);
}

void GameUI_DrawCharacterSelect(uint8_t selectedCharacter, uint8_t cpuCharacter)
{
  selectedCharacter %= CHOOSE_CHARACTER_COUNT;
  cpuCharacter %= CHOOSE_CHARACTER_COUNT;

  LCD_Port_DrawRGB565Bytes(0U,
                           0U,
                           CHOOSE_CHAR_WIDTH,
                           CHOOSE_CHAR_HEIGHT,
                           choose_char_map);

  ILI9341_DrawFilledRectangleCoord(37U, 12U, 283U, 39U, UI_COLOR_SHADOW);
  ILI9341_DrawHollowRectangleCoord(37U, 12U, 283U, 39U, UI_COLOR_ORANGE);
  ILI9341_DrawText("> SELECT CHARACTER <", FONT3, 52U, 18U, UI_COLOR_YELLOW, UI_COLOR_SHADOW);

  GameUI_DrawCharacterGrid();
  GameUI_DrawCharacterBanners(selectedCharacter, cpuCharacter);
  GameUI_DrawCharacterCursor(selectedCharacter);
}

void GameUI_UpdateCharacterSelect(uint8_t previousCharacter,
                                  uint8_t selectedCharacter,
                                  uint8_t cpuCharacter)
{
  previousCharacter %= CHOOSE_CHARACTER_COUNT;
  selectedCharacter %= CHOOSE_CHARACTER_COUNT;
  cpuCharacter %= CHOOSE_CHARACTER_COUNT;

  if (previousCharacter != selectedCharacter)
  {
    uint16_t oldX = (uint16_t)(GameUI_GetCharacterAvatarX(previousCharacter) - CHARACTER_CURSOR_PAD);
    uint16_t oldY = (uint16_t)(GameUI_GetCharacterAvatarY(previousCharacter) - CHARACTER_CURSOR_PAD);

    GameUI_DrawChooseBackgroundRect(oldX,
                                    oldY,
                                    CHARACTER_CURSOR_SIZE,
                                    CHARACTER_CURSOR_SIZE);
    GameUI_DrawCharacterAvatar(previousCharacter);
  }

  GameUI_DrawCharacterBanners(selectedCharacter, cpuCharacter);
  GameUI_DrawCharacterCursor(selectedCharacter);
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

static const char *GameUI_GetDifficultyMenuLabel(uint8_t difficulty)
{
  switch (difficulty % 3U)
  {
    case 0U:
      return "DIFFICULTY: EASY";
    case 2U:
      return "DIFFICULTY: HARD";
    default:
      return "DIFFICULTY: NORMAL";
  }
}

static const char *GameUI_GetCharacterMenuLabel(uint8_t character)
{
  switch (character % CHOOSE_CHARACTER_COUNT)
  {
    case CHOOSE_CHARACTER_NARUTO:
      return "CHARACTER: NARUTO";
    case CHOOSE_CHARACTER_NINE_TAIL:
      return "CHARACTER: NINE";
    case CHOOSE_CHARACTER_SASUKE:
      return "CHARACTER: SASUKE";
    case CHOOSE_CHARACTER_ICHIGO:
      return "CHARACTER: ICHIGO";
    case CHOOSE_CHARACTER_GIN:
      return "CHARACTER: GIN";
    case CHOOSE_CHARACTER_HOLLOW:
    default:
      return "CHARACTER: VIZARD";
  }
}

static void GameUI_DrawDifficultyOption(uint16_t y,
                                        const char *label,
                                        uint8_t selected)
{
  uint16_t borderColor = selected ? UI_COLOR_YELLOW : UI_COLOR_WHITE;
  uint16_t textColor = selected ? UI_COLOR_BLACK : UI_COLOR_WHITE;
  uint16_t fillColor = selected ? UI_COLOR_ORANGE : UI_COLOR_PANEL_DARK;
  uint16_t labelX = selected ? 123U : 136U;

  ILI9341_DrawFilledRectangleCoord(75U, y, 245U, (uint16_t)(y + 25U), fillColor);
  ILI9341_DrawHollowRectangleCoord(75U, y, 245U, (uint16_t)(y + 25U), borderColor);

  if (selected != 0U)
  {
    ILI9341_DrawText(">", FONT3, 94U, (uint16_t)(y + 5U), UI_COLOR_BLACK, fillColor);
  }

  ILI9341_DrawText(label, FONT3, labelX, (uint16_t)(y + 5U), textColor, fillColor);
}

static void GameUI_DrawChooseBackgroundRect(uint16_t x,
                                            uint16_t y,
                                            uint16_t width,
                                            uint16_t height)
{
  uint16_t row;

  if ((width == 0U) || (height == 0U))
  {
    return;
  }

  for (row = 0U; row < height; row++)
  {
    const uint8_t *line = &choose_char_map[((((uint32_t)y + row) * CHOOSE_CHAR_WIDTH) + x) * 2U];
    LCD_Port_DrawRGB565Bytes(x, (uint16_t)(y + row), width, 1U, line);
  }
}

static void GameUI_DrawCharacterGrid(void)
{
  uint8_t character;

  for (character = 0U; character < CHOOSE_CHARACTER_COUNT; character++)
  {
    GameUI_DrawCharacterAvatar(character);
  }
}

static void GameUI_DrawCharacterAvatar(uint8_t character)
{
  character %= CHOOSE_CHARACTER_COUNT;

  LCD_Port_DrawRGB565Bytes(GameUI_GetCharacterAvatarX(character),
                           GameUI_GetCharacterAvatarY(character),
                           CHOOSE_AVATAR_WIDTH,
                           CHOOSE_AVATAR_HEIGHT,
                           choose_avatar_maps[character]);
}

static void GameUI_DrawCharacterCursor(uint8_t character)
{
  uint16_t x;
  uint16_t y;

  character %= CHOOSE_CHARACTER_COUNT;
  x = (uint16_t)(GameUI_GetCharacterAvatarX(character) - CHARACTER_CURSOR_PAD);
  y = (uint16_t)(GameUI_GetCharacterAvatarY(character) - CHARACTER_CURSOR_PAD);

  ILI9341_DrawHollowRectangleCoord(x,
                                   y,
                                   (uint16_t)(x + CHARACTER_CURSOR_SIZE - 1U),
                                   (uint16_t)(y + CHARACTER_CURSOR_SIZE - 1U),
                                   UI_COLOR_YELLOW);
  ILI9341_DrawHollowRectangleCoord((uint16_t)(x + 1U),
                                   (uint16_t)(y + 1U),
                                   (uint16_t)(x + CHARACTER_CURSOR_SIZE - 2U),
                                   (uint16_t)(y + CHARACTER_CURSOR_SIZE - 2U),
                                   UI_COLOR_ORANGE);
}

static void GameUI_DrawCharacterBanners(uint8_t selectedCharacter,
                                        uint8_t cpuCharacter)
{
  selectedCharacter %= CHOOSE_CHARACTER_COUNT;
  cpuCharacter %= CHOOSE_CHARACTER_COUNT;

  LCD_Port_DrawRGB565Bytes(CHARACTER_P1_BANNER_X,
                           CHARACTER_BANNER_Y,
                           CHOOSE_BANNER_WIDTH,
                           CHOOSE_BANNER_HEIGHT,
                           choose_banner_maps[selectedCharacter]);
  LCD_Port_DrawRGB565Bytes(CHARACTER_CPU_BANNER_X,
                           CHARACTER_BANNER_Y,
                           CHOOSE_BANNER_WIDTH,
                           CHOOSE_BANNER_HEIGHT,
                           choose_banner_maps[cpuCharacter]);

  ILI9341_DrawHollowRectangleCoord(CHARACTER_P1_BANNER_X,
                                   CHARACTER_BANNER_Y,
                                   (uint16_t)(CHARACTER_P1_BANNER_X + CHOOSE_BANNER_WIDTH - 1U),
                                   (uint16_t)(CHARACTER_BANNER_Y + CHOOSE_BANNER_HEIGHT - 1U),
                                   UI_COLOR_CYAN);
  ILI9341_DrawHollowRectangleCoord(CHARACTER_CPU_BANNER_X,
                                   CHARACTER_BANNER_Y,
                                   (uint16_t)(CHARACTER_CPU_BANNER_X + CHOOSE_BANNER_WIDTH - 1U),
                                   (uint16_t)(CHARACTER_BANNER_Y + CHOOSE_BANNER_HEIGHT - 1U),
                                   UI_COLOR_RED);
  ILI9341_DrawText("P1", FONT1, 12U, 187U, UI_COLOR_CYAN, UI_COLOR_BLACK);
  ILI9341_DrawText("CPU", FONT1, 172U, 187U, UI_COLOR_RED, UI_COLOR_BLACK);
}

static uint16_t GameUI_GetCharacterAvatarX(uint8_t character)
{
  uint8_t column = (uint8_t)(character % 3U);

  return (uint16_t)(CHARACTER_GRID_X0 + ((CHOOSE_AVATAR_WIDTH + CHARACTER_GRID_GAP_X) * column));
}

static uint16_t GameUI_GetCharacterAvatarY(uint8_t character)
{
  uint8_t row = (uint8_t)((character % CHOOSE_CHARACTER_COUNT) / 3U);

  return (uint16_t)(CHARACTER_GRID_Y0 + ((CHOOSE_AVATAR_HEIGHT + CHARACTER_GRID_GAP_Y) * row));
}
