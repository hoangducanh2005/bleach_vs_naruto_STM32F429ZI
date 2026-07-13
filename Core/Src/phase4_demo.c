/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : phase4_demo.c
  * @brief          : Phase 4 flow: splash screen to two-character battle demo.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "phase4_demo.h"

#include "choose_ui_assets.h"
#include "game_ui.h"
#include "ILI9341_GFX.h"
#include "lcd_port.h"
#include "menu_joystick.h"
#include "naruto_moveset.h"
#include "sasuke_moveset.h"
#include "sprite_data.h"
#include "sprite_render.h"
#include "stm32f4xx_hal.h"

#define PHASE4_FRAME_TIME_MS 33U
#define PHASE4_SPLASH_AUTO_MS 3000U
#define PHASE4_GROUND_Y 205
#define PHASE4_P1_ANCHOR_X 88
#define PHASE4_P2_ANCHOR_X 232

#define RGB565_BLACK 0x0000U
#define RGB565_WHITE 0xFFFFU
#define RGB565_SKY_TOP 0x5D7FU
#define RGB565_SKY_MID 0x865FU
#define RGB565_SKY_LOW 0xB73FU
#define RGB565_CLOUD 0xF7BEU
#define RGB565_CLOUD_SHADOW 0xCE79U
#define RGB565_SUN 0xFFE0U
#define RGB565_MOUNTAIN_DARK 0x4A69U
#define RGB565_MOUNTAIN_LIGHT 0x6B6DU
#define RGB565_WALL_DARK 0x4208U
#define RGB565_WALL_LIGHT 0x632CU
#define RGB565_GROUND_TOP 0x7BEFU
#define RGB565_GROUND 0x39E7U
#define RGB565_GROUND_DARK 0x2104U
#define RGB565_LINE 0xEF5DU
#define RGB565_ACCENT_ORANGE 0xFD20U
#define RGB565_ACCENT_CYAN 0x07FFU
#define RGB565_LABEL_BG 0x0008U

typedef enum
{
  PHASE4_SCREEN_SPLASH = 0U,
  PHASE4_SCREEN_MAIN_MENU = 1U,
  PHASE4_SCREEN_CHARACTER_SELECT = 2U,
  PHASE4_SCREEN_DIFFICULTY_SELECT = 3U,
  PHASE4_SCREEN_BATTLE = 4U
} Phase4Screen;

static Phase4Screen s_screen;
static uint32_t s_screenStartedMs;
static uint32_t s_lastFrameMs;
static uint32_t s_battleStartedMs;
static uint8_t s_selectedMenu;
static uint8_t s_selectedDifficulty;
static uint8_t s_selectedCharacter;
static uint8_t s_cpuCharacter;
static uint8_t s_narutoFrameIndex;
static uint8_t s_sasukeFrameIndex;
static uint8_t s_narutoPreviousValid;
static uint8_t s_sasukePreviousValid;
static const NarutoMoveFrame *s_narutoPreviousFrame;
static const SasukeMoveFrame *s_sasukePreviousFrame;
static int16_t s_narutoPreviousX;
static int16_t s_narutoPreviousY;
static int16_t s_sasukePreviousX;
static int16_t s_sasukePreviousY;

static void Phase4_ShowSplash(void);
static void Phase4_ShowMainMenu(void);
static void Phase4_ShowCharacterSelect(void);
static void Phase4_ShowDifficultySelect(void);
static void Phase4_HandleMenuEvent(MenuJoystickEvent event);
static void Phase4_HandleCharacterSelectEvent(MenuJoystickEvent event);
static void Phase4_HandleDifficultySelectEvent(MenuJoystickEvent event);
static void Phase4_StartBattle(void);
static void Phase4_UpdateBattle(uint32_t nowMs);
static uint8_t Phase4_GetCpuCharacter(uint8_t selectedCharacter);
static void Phase4_DrawBackground(void);
static void Phase4_DrawHud(void);
static void Phase4_DrawCharacters(void);
static void Phase4_UpdateNaruto(uint32_t elapsedMs);
static void Phase4_UpdateSasuke(uint32_t elapsedMs);
static uint8_t Phase4_GetFrameIndex(uint8_t frameCount,
                                    uint16_t frameDurationMs,
                                    uint32_t elapsedMs);
static void Phase4_DrawStaticSky(void);
static void Phase4_DrawSun(uint16_t x, uint16_t y);
static void Phase4_DrawCloud(int16_t x, uint16_t y);
static void Phase4_DrawMountains(void);
static void Phase4_DrawDojoWall(void);
static void Phase4_DrawGround(void);
static void Phase4_DrawArenaMarks(void);
static uint16_t Phase4_GetBackgroundPixel(uint16_t x, uint16_t y);
static void Phase4_FillSafe(int16_t x,
                            int16_t y,
                            int16_t width,
                            int16_t height,
                            uint16_t color);

void Phase4Demo_Init(void)
{
  LCD_Port_Init();
  MenuJoystick_Init();
  s_selectedMenu = 0U;
  s_selectedDifficulty = 1U;
  s_selectedCharacter = CHOOSE_CHARACTER_NARUTO;
  s_cpuCharacter = Phase4_GetCpuCharacter(s_selectedCharacter);
  Phase4_ShowSplash();
}

void Phase4Demo_Update(void)
{
  uint32_t now = HAL_GetTick();
  MenuJoystickEvent event = MenuJoystick_ReadEvent();

  if (s_screen == PHASE4_SCREEN_SPLASH)
  {
    if ((now - s_screenStartedMs) >= PHASE4_SPLASH_AUTO_MS)
    {
      Phase4_ShowMainMenu();
    }
    return;
  }

  if (s_screen == PHASE4_SCREEN_MAIN_MENU)
  {
    Phase4_HandleMenuEvent(event);
    return;
  }

  if (s_screen == PHASE4_SCREEN_CHARACTER_SELECT)
  {
    Phase4_HandleCharacterSelectEvent(event);
    return;
  }

  if (s_screen == PHASE4_SCREEN_DIFFICULTY_SELECT)
  {
    Phase4_HandleDifficultySelectEvent(event);
    return;
  }

  Phase4_UpdateBattle(now);
}

static void Phase4_ShowSplash(void)
{
  s_screen = PHASE4_SCREEN_SPLASH;
  s_screenStartedMs = HAL_GetTick();
  GameUI_DrawSplash();
  LCD_Port_Flush();
}

static void Phase4_ShowMainMenu(void)
{
  s_screen = PHASE4_SCREEN_MAIN_MENU;
  s_screenStartedMs = HAL_GetTick();
  GameUI_DrawMainMenuSelection(s_selectedMenu,
                               s_selectedDifficulty,
                               s_selectedCharacter);
  LCD_Port_Flush();
}

static void Phase4_ShowCharacterSelect(void)
{
  s_screen = PHASE4_SCREEN_CHARACTER_SELECT;
  s_screenStartedMs = HAL_GetTick();
  GameUI_DrawCharacterSelect(s_selectedCharacter, s_cpuCharacter);
  ILI9341_DrawText("Y DOWN: NEXT  Y UP: OK", FONT1, 72U, 229U, RGB565_WHITE, RGB565_BLACK);
  LCD_Port_Flush();
}

static void Phase4_ShowDifficultySelect(void)
{
  s_screen = PHASE4_SCREEN_DIFFICULTY_SELECT;
  s_screenStartedMs = HAL_GetTick();
  GameUI_DrawDifficultySelect(s_selectedDifficulty);
  ILI9341_DrawText("Y DOWN: NEXT  Y UP: OK", FONT1, 72U, 229U, RGB565_WHITE, RGB565_BLACK);
  LCD_Port_Flush();
}

static void Phase4_HandleMenuEvent(MenuJoystickEvent event)
{
  if (event == MENU_JOYSTICK_EVENT_NEXT)
  {
    uint8_t previousMenu = s_selectedMenu;
    s_selectedMenu = (uint8_t)((s_selectedMenu + 1U) % 3U);
    GameUI_UpdateMainMenuSelection(previousMenu,
                                   s_selectedMenu,
                                   s_selectedDifficulty,
                                   s_selectedCharacter);
    LCD_Port_Flush();
  }
  else if (event == MENU_JOYSTICK_EVENT_CONFIRM)
  {
    if (s_selectedMenu == 0U)
    {
      Phase4_StartBattle();
    }
    else if (s_selectedMenu == 1U)
    {
      Phase4_ShowDifficultySelect();
    }
    else
    {
      Phase4_ShowCharacterSelect();
    }
  }
}

static void Phase4_HandleCharacterSelectEvent(MenuJoystickEvent event)
{
  if (event == MENU_JOYSTICK_EVENT_NEXT)
  {
    uint8_t previousCharacter = s_selectedCharacter;
    s_selectedCharacter =
        (uint8_t)((s_selectedCharacter + 1U) % CHOOSE_CHARACTER_COUNT);
    s_cpuCharacter = Phase4_GetCpuCharacter(s_selectedCharacter);
    GameUI_UpdateCharacterSelect(previousCharacter,
                                 s_selectedCharacter,
                                 s_cpuCharacter);
    ILI9341_DrawText("Y DOWN: NEXT  Y UP: OK", FONT1, 72U, 229U, RGB565_WHITE, RGB565_BLACK);
    LCD_Port_Flush();
  }
  else if (event == MENU_JOYSTICK_EVENT_CONFIRM)
  {
    Phase4_ShowMainMenu();
  }
}

static void Phase4_HandleDifficultySelectEvent(MenuJoystickEvent event)
{
  if (event == MENU_JOYSTICK_EVENT_NEXT)
  {
    s_selectedDifficulty = (uint8_t)((s_selectedDifficulty + 1U) % 3U);
    GameUI_DrawDifficultySelect(s_selectedDifficulty);
    ILI9341_DrawText("Y DOWN: NEXT  Y UP: OK", FONT1, 72U, 229U, RGB565_WHITE, RGB565_BLACK);
    LCD_Port_Flush();
  }
  else if (event == MENU_JOYSTICK_EVENT_CONFIRM)
  {
    Phase4_ShowMainMenu();
  }
}

static void Phase4_StartBattle(void)
{
  s_screen = PHASE4_SCREEN_BATTLE;
  s_screenStartedMs = HAL_GetTick();
  s_lastFrameMs = HAL_GetTick();
  s_battleStartedMs = s_lastFrameMs;
  s_narutoFrameIndex = 0U;
  s_sasukeFrameIndex = 0U;
  s_narutoPreviousValid = 0U;
  s_sasukePreviousValid = 0U;

  Phase4_DrawBackground();
  Phase4_DrawHud();
  Phase4_DrawCharacters();
  LCD_Port_Flush();
}

static uint8_t Phase4_GetCpuCharacter(uint8_t selectedCharacter)
{
  return (uint8_t)((selectedCharacter + 2U) % CHOOSE_CHARACTER_COUNT);
}

static void Phase4_UpdateBattle(uint32_t nowMs)
{
  if ((nowMs - s_lastFrameMs) < PHASE4_FRAME_TIME_MS)
  {
    return;
  }

  s_lastFrameMs += PHASE4_FRAME_TIME_MS;

  uint32_t elapsedMs = nowMs - s_battleStartedMs;
  Phase4_UpdateNaruto(elapsedMs);
  Phase4_UpdateSasuke(elapsedMs);
  LCD_Port_Flush();
}

static void Phase4_DrawCharacters(void)
{
  const NarutoMoveAnimation *narutoAnimation =
      &naruto_move_animations[NARUTO_MOVE_IDLE];
  const NarutoMoveFrame *narutoFrame = &narutoAnimation->frames[s_narutoFrameIndex];
  int16_t narutoX = (int16_t)(PHASE4_P1_ANCHOR_X - narutoFrame->pivotX);
  int16_t narutoY = (int16_t)(PHASE4_GROUND_Y - narutoFrame->pivotY);

  const SasukeMoveAnimation *sasukeAnimation =
      &sasuke_move_animations[SASUKE_MOVE_IDLE];
  const SasukeMoveFrame *sasukeFrame = &sasukeAnimation->frames[s_sasukeFrameIndex];
  int16_t sasukeX = (int16_t)(PHASE4_P2_ANCHOR_X - sasukeFrame->pivotX);
  int16_t sasukeY = (int16_t)(PHASE4_GROUND_Y - sasukeFrame->pivotY);

  SpriteRender_Draw(narutoX,
                    narutoY,
                    narutoFrame->pixels,
                    narutoFrame->width,
                    narutoFrame->height,
                    0U);
  SpriteRender_Draw(sasukeX,
                    sasukeY,
                    sasukeFrame->pixels,
                    sasukeFrame->width,
                    sasukeFrame->height,
                    1U);

  s_narutoPreviousFrame = narutoFrame;
  s_narutoPreviousX = narutoX;
  s_narutoPreviousY = narutoY;
  s_narutoPreviousValid = 1U;

  s_sasukePreviousFrame = sasukeFrame;
  s_sasukePreviousX = sasukeX;
  s_sasukePreviousY = sasukeY;
  s_sasukePreviousValid = 1U;
}

static void Phase4_UpdateNaruto(uint32_t elapsedMs)
{
  const NarutoMoveAnimation *animation =
      &naruto_move_animations[NARUTO_MOVE_IDLE];
  uint8_t nextFrame = Phase4_GetFrameIndex(animation->frameCount,
                                          animation->frames[0].durationMs,
                                          elapsedMs);

  if (nextFrame == s_narutoFrameIndex)
  {
    return;
  }

  if (s_narutoPreviousValid != 0U)
  {
    SpriteRender_Erase(s_narutoPreviousX,
                       s_narutoPreviousY,
                       s_narutoPreviousFrame->pixels,
                       s_narutoPreviousFrame->width,
                       s_narutoPreviousFrame->height,
                       0U,
                       Phase4_GetBackgroundPixel);
  }

  s_narutoFrameIndex = nextFrame;
  Phase4_DrawCharacters();
}

static void Phase4_UpdateSasuke(uint32_t elapsedMs)
{
  const SasukeMoveAnimation *animation =
      &sasuke_move_animations[SASUKE_MOVE_IDLE];
  uint8_t nextFrame = Phase4_GetFrameIndex(animation->frameCount,
                                          animation->frames[0].durationMs,
                                          elapsedMs);

  if (nextFrame == s_sasukeFrameIndex)
  {
    return;
  }

  if (s_sasukePreviousValid != 0U)
  {
    SpriteRender_Erase(s_sasukePreviousX,
                       s_sasukePreviousY,
                       s_sasukePreviousFrame->pixels,
                       s_sasukePreviousFrame->width,
                       s_sasukePreviousFrame->height,
                       1U,
                       Phase4_GetBackgroundPixel);
  }

  s_sasukeFrameIndex = nextFrame;
  Phase4_DrawCharacters();
}

static uint8_t Phase4_GetFrameIndex(uint8_t frameCount,
                                    uint16_t frameDurationMs,
                                    uint32_t elapsedMs)
{
  if ((frameCount == 0U) || (frameDurationMs == 0U))
  {
    return 0U;
  }

  return (uint8_t)((elapsedMs / frameDurationMs) % frameCount);
}

static void Phase4_DrawBackground(void)
{
  Phase4_DrawStaticSky();
  Phase4_DrawSun(268U, 34U);
  Phase4_DrawCloud(30, 36U);
  Phase4_DrawCloud(178, 58U);
  Phase4_DrawMountains();
  Phase4_DrawDojoWall();
  Phase4_DrawGround();
  Phase4_DrawArenaMarks();
}

static void Phase4_DrawHud(void)
{
  ILI9341_DrawFilledRectangleCoord(6U, 6U, 150U, 26U, RGB565_LABEL_BG);
  ILI9341_DrawHollowRectangleCoord(6U, 6U, 150U, 26U, RGB565_ACCENT_CYAN);
  ILI9341_DrawText("P1 NARUTO", FONT2, 16U, 11U, RGB565_WHITE, RGB565_LABEL_BG);

  ILI9341_DrawFilledRectangleCoord(170U, 6U, 314U, 26U, RGB565_LABEL_BG);
  ILI9341_DrawHollowRectangleCoord(170U, 6U, 314U, 26U, RGB565_ACCENT_ORANGE);
  ILI9341_DrawText("CPU SASUKE", FONT2, 181U, 11U, RGB565_WHITE, RGB565_LABEL_BG);

  ILI9341_DrawFilledRectangleCoord(137U, 8U, 183U, 28U, RGB565_BLACK);
  ILI9341_DrawText("VS", FONT3, 151U, 12U, RGB565_LINE, RGB565_BLACK);
}

static uint16_t Phase4_GetBackgroundPixel(uint16_t x, uint16_t y)
{
  uint16_t color = RGB565_SKY_LOW;

  if (y < 48U)
  {
    color = RGB565_SKY_TOP;
  }
  else if (y < 96U)
  {
    color = RGB565_SKY_MID;
  }
  else if (y < 148U)
  {
    uint16_t h1 = (uint16_t)(22U + ((x * 17U) % 39U));
    uint16_t h2 = (uint16_t)(14U + ((x * 11U) % 31U));

    if (y >= (uint16_t)(148U - h2))
    {
      color = RGB565_MOUNTAIN_LIGHT;
    }
    else if (y >= (uint16_t)(148U - h1))
    {
      color = RGB565_MOUNTAIN_DARK;
    }
  }
  else if (y < 182U)
  {
    color = RGB565_WALL_DARK;

    if ((x % 32U) < 2U)
    {
      color = RGB565_WALL_LIGHT;
    }

    if ((y >= 180U) || (y < 150U))
    {
      color = RGB565_BLACK;
    }
  }
  else
  {
    color = RGB565_GROUND;

    if (y < 188U)
    {
      color = RGB565_GROUND_TOP;
    }
    else if (y >= 232U)
    {
      color = RGB565_GROUND_DARK;
    }

    if ((y >= 190U) && (y < 192U) && (x >= 40U) && (x < 280U))
    {
      color = RGB565_LINE;
    }

    if ((x >= 158U) && (x < 162U) && (y >= 186U) && (y < 228U))
    {
      color = RGB565_LINE;
    }

    if ((x >= 58U) && (x < 100U) && (y >= 199U) && (y < 203U))
    {
      color = RGB565_ACCENT_CYAN;
    }

    if ((x >= 220U) && (x < 262U) && (y >= 199U) && (y < 203U))
    {
      color = RGB565_ACCENT_ORANGE;
    }
  }

  return color;
}

static void Phase4_DrawStaticSky(void)
{
  LCD_Port_FillRect(0U, 0U, LCD_PORT_WIDTH, 48U, RGB565_SKY_TOP);
  LCD_Port_FillRect(0U, 48U, LCD_PORT_WIDTH, 48U, RGB565_SKY_MID);
  LCD_Port_FillRect(0U, 96U, LCD_PORT_WIDTH, 52U, RGB565_SKY_LOW);
}

static void Phase4_DrawSun(uint16_t x, uint16_t y)
{
  Phase4_FillSafe((int16_t)x - 10, (int16_t)y - 10, 20, 20, RGB565_SUN);
  Phase4_FillSafe((int16_t)x - 14, (int16_t)y - 4, 28, 8, RGB565_SUN);
  Phase4_FillSafe((int16_t)x - 4, (int16_t)y - 14, 8, 28, RGB565_SUN);
}

static void Phase4_DrawCloud(int16_t x, uint16_t y)
{
  Phase4_FillSafe(x + 8, (int16_t)y + 7, 45, 9, RGB565_CLOUD_SHADOW);
  Phase4_FillSafe(x, (int16_t)y + 8, 24, 9, RGB565_CLOUD);
  Phase4_FillSafe(x + 13, (int16_t)y + 2, 27, 15, RGB565_CLOUD);
  Phase4_FillSafe(x + 34, (int16_t)y + 7, 27, 10, RGB565_CLOUD);
  Phase4_FillSafe(x + 7, (int16_t)y + 15, 48, 5, RGB565_CLOUD);
}

static void Phase4_DrawMountains(void)
{
  for (uint16_t x = 0U; x < LCD_PORT_WIDTH; x += 8U)
  {
    uint16_t h1 = (uint16_t)(22U + ((x * 17U) % 39U));
    uint16_t h2 = (uint16_t)(14U + ((x * 11U) % 31U));
    Phase4_FillSafe((int16_t)x,
                    (int16_t)(148U - h1),
                    8,
                    (int16_t)h1,
                    RGB565_MOUNTAIN_DARK);
    Phase4_FillSafe((int16_t)x,
                    (int16_t)(148U - h2),
                    4,
                    (int16_t)h2,
                    RGB565_MOUNTAIN_LIGHT);
  }
}

static void Phase4_DrawDojoWall(void)
{
  LCD_Port_FillRect(0U, 148U, LCD_PORT_WIDTH, 34U, RGB565_WALL_DARK);

  for (uint16_t x = 0U; x < LCD_PORT_WIDTH; x += 32U)
  {
    LCD_Port_FillRect(x, 148U, 2U, 34U, RGB565_WALL_LIGHT);
  }

  LCD_Port_FillRect(0U, 148U, LCD_PORT_WIDTH, 2U, RGB565_WALL_LIGHT);
  LCD_Port_FillRect(0U, 180U, LCD_PORT_WIDTH, 2U, RGB565_BLACK);
}

static void Phase4_DrawGround(void)
{
  LCD_Port_FillRect(0U, 182U, LCD_PORT_WIDTH, 58U, RGB565_GROUND);
  LCD_Port_FillRect(0U, 182U, LCD_PORT_WIDTH, 6U, RGB565_GROUND_TOP);
  LCD_Port_FillRect(0U, 232U, LCD_PORT_WIDTH, 8U, RGB565_GROUND_DARK);
}

static void Phase4_DrawArenaMarks(void)
{
  LCD_Port_FillRect(40U, 190U, 240U, 2U, RGB565_LINE);
  LCD_Port_FillRect(158U, 186U, 4U, 42U, RGB565_LINE);

  Phase4_FillSafe(58, 199, 42, 4, RGB565_ACCENT_CYAN);
  Phase4_FillSafe(220, 199, 42, 4, RGB565_ACCENT_ORANGE);
}

static void Phase4_FillSafe(int16_t x,
                            int16_t y,
                            int16_t width,
                            int16_t height,
                            uint16_t color)
{
  if ((width <= 0) || (height <= 0))
  {
    return;
  }

  int16_t x0 = x;
  int16_t y0 = y;
  int16_t x1 = (int16_t)(x + width);
  int16_t y1 = (int16_t)(y + height);

  if (x0 < 0)
  {
    x0 = 0;
  }
  if (y0 < 0)
  {
    y0 = 0;
  }
  if (x1 > (int16_t)LCD_PORT_WIDTH)
  {
    x1 = (int16_t)LCD_PORT_WIDTH;
  }
  if (y1 > (int16_t)LCD_PORT_HEIGHT)
  {
    y1 = (int16_t)LCD_PORT_HEIGHT;
  }

  if ((x1 <= x0) || (y1 <= y0))
  {
    return;
  }

  LCD_Port_FillRect((uint16_t)x0,
                    (uint16_t)y0,
                    (uint16_t)(x1 - x0),
                    (uint16_t)(y1 - y0),
                    color);
}
