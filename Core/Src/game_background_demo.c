/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : game_background_demo.c
  * @brief          : Lightweight 30 FPS background drawing demo.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "game_background_demo.h"

#include "lcd_port.h"
#include "sprite_data.h"
#include "sprite_render.h"
#include "stm32f4xx_hal.h"

#define GAME_FPS               30U
#define FRAME_TIME_MS          (1000U / GAME_FPS)
#define BACKGROUND_ANIMATION_ENABLED  0U

#define DEMO_GROUND_Y          190
#define ICHIGO_SKILL_X         50
#define ICHIGO_SKILL_Y         (DEMO_GROUND_Y - (int16_t)ICHIGO_GETSUGA_HEIGHT)
#define ICHIGO_SKILL_TICKS_PER_FRAME 1U
#define ICHIGO_PROJECTILE_SPAWN_FRAME 3U
#define NARUTO_IDLE_X          220
#define NARUTO_IDLE_Y          (DEMO_GROUND_Y - (int16_t)NARUTO_IDLE_HEIGHT)
#define NARUTO_TICKS_PER_FRAME 2U
#define GETSUGA_START_X        (ICHIGO_SKILL_X + 55)
#define GETSUGA_START_Y        (ICHIGO_SKILL_Y + 18)
#define GETSUGA_SPEED_X        35
#define GETSUGA_TICKS_PER_FRAME 1U

#define RGB565_BLACK           0x0000U
#define RGB565_WHITE           0xFFFFU
#define RGB565_SKY_TOP         0x5D7FU
#define RGB565_SKY_MID         0x865FU
#define RGB565_SKY_LOW         0xB73FU
#define RGB565_CLOUD           0xF7BEU
#define RGB565_CLOUD_SHADOW    0xCE79U
#define RGB565_SUN             0xFFE0U
#define RGB565_MOUNTAIN_DARK   0x4A69U
#define RGB565_MOUNTAIN_LIGHT  0x6B6DU
#define RGB565_WALL_DARK       0x4208U
#define RGB565_WALL_LIGHT      0x632CU
#define RGB565_GROUND_TOP      0x7BEFU
#define RGB565_GROUND          0x39E7U
#define RGB565_GROUND_DARK     0x2104U
#define RGB565_LINE            0xEF5DU
#define RGB565_ACCENT_ORANGE   0xFD20U
#define RGB565_ACCENT_CYAN     0x07FFU

static uint32_t s_lastFrameMs;
static uint32_t s_frameCounter;
static uint8_t s_ichigoSkillFrameIndex;
static uint8_t s_narutoFrameIndex;
static int16_t s_getsugaX;
static int16_t s_getsugaY;
static uint8_t s_getsugaFrameIndex;
static uint8_t s_getsugaActive;

static void Demo_DrawBackground(uint32_t frame);
static void Demo_DrawIchigo(uint8_t frameIndex);
static void Demo_DrawNaruto(uint8_t frameIndex);
static void Demo_UpdateIchigo(void);
static void Demo_UpdateNaruto(void);
static void Demo_SpawnGetsuga(void);
static void Demo_UpdateGetsuga(void);
static uint16_t Demo_GetBackgroundPixel(uint16_t x, uint16_t y);
static void Demo_DrawStaticSky(void);
static void Demo_DrawSun(uint16_t x, uint16_t y);
static void Demo_DrawCloud(int16_t x, uint16_t y);
static void Demo_DrawMountains(void);
static void Demo_DrawDojoWall(void);
static void Demo_DrawGround(uint32_t frame);
static void Demo_DrawArenaMarks(uint32_t frame);
static void Demo_FillSafe(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color);

void GameBackgroundDemo_Init(void)
{
  LCD_Port_Init();
  s_lastFrameMs = HAL_GetTick();
  s_frameCounter = 0U;
  s_ichigoSkillFrameIndex = 0U;
  s_narutoFrameIndex = 0U;
  s_getsugaActive = 0U;
  s_getsugaFrameIndex = 0U;
  s_getsugaX = GETSUGA_START_X;
  s_getsugaY = GETSUGA_START_Y;
  Demo_DrawBackground(s_frameCounter);
  Demo_DrawIchigo(s_ichigoSkillFrameIndex);
  Demo_DrawNaruto(s_narutoFrameIndex);
  LCD_Port_Flush();
}

void GameBackgroundDemo_Update(void)
{
  uint32_t now = HAL_GetTick();

  if ((now - s_lastFrameMs) < FRAME_TIME_MS)
  {
    return;
  }

  s_lastFrameMs += FRAME_TIME_MS;
  s_frameCounter++;

#if BACKGROUND_ANIMATION_ENABLED
  Demo_DrawBackground(s_frameCounter);
#else
  Demo_UpdateIchigo();
  Demo_UpdateGetsuga();
  Demo_UpdateNaruto();
#endif

  LCD_Port_Flush();
}

static void Demo_DrawBackground(uint32_t frame)
{
  Demo_DrawStaticSky();
  Demo_DrawSun(268U, 34U);
  Demo_DrawCloud((int16_t)(24 + (int16_t)((frame / 3U) % 42U)), 36U);
  Demo_DrawCloud((int16_t)(178 - (int16_t)((frame / 4U) % 36U)), 58U);
  Demo_DrawMountains();
  Demo_DrawDojoWall();
  Demo_DrawGround(frame);
  Demo_DrawArenaMarks(frame);
}

static void Demo_DrawIchigo(uint8_t frameIndex)
{
  SpriteRender_Draw(ICHIGO_SKILL_X,
                    ICHIGO_SKILL_Y,
                    ichigo_getsuga_frames[frameIndex],
                    ICHIGO_GETSUGA_WIDTH,
                    ICHIGO_GETSUGA_HEIGHT,
                    0U);
}

static void Demo_DrawNaruto(uint8_t frameIndex)
{
  SpriteRender_Draw(NARUTO_IDLE_X,
                    NARUTO_IDLE_Y,
                    naruto_idle_frames[frameIndex],
                    NARUTO_IDLE_WIDTH,
                    NARUTO_IDLE_HEIGHT,
                    1U);
}

static void Demo_UpdateIchigo(void)
{
  uint8_t nextFrame = (uint8_t)((s_frameCounter / ICHIGO_SKILL_TICKS_PER_FRAME) % ICHIGO_GETSUGA_FRAME_COUNT);

  if (nextFrame == s_ichigoSkillFrameIndex)
  {
    return;
  }

  SpriteRender_DrawDiff(ICHIGO_SKILL_X,
                        ICHIGO_SKILL_Y,
                        ichigo_getsuga_frames[s_ichigoSkillFrameIndex],
                        ichigo_getsuga_frames[nextFrame],
                        ICHIGO_GETSUGA_WIDTH,
                        ICHIGO_GETSUGA_HEIGHT,
                        0U,
                        Demo_GetBackgroundPixel);

  if (nextFrame == ICHIGO_PROJECTILE_SPAWN_FRAME)
  {
    Demo_SpawnGetsuga();
  }

  s_ichigoSkillFrameIndex = nextFrame;
}

static void Demo_UpdateNaruto(void)
{
  uint8_t nextFrame = (uint8_t)((s_frameCounter / NARUTO_TICKS_PER_FRAME) % NARUTO_IDLE_FRAME_COUNT);

  if (nextFrame == s_narutoFrameIndex)
  {
    return;
  }

  SpriteRender_DrawDiff(NARUTO_IDLE_X,
                        NARUTO_IDLE_Y,
                        naruto_idle_frames[s_narutoFrameIndex],
                        naruto_idle_frames[nextFrame],
                        NARUTO_IDLE_WIDTH,
                        NARUTO_IDLE_HEIGHT,
                        1U,
                        Demo_GetBackgroundPixel);
  s_narutoFrameIndex = nextFrame;
}

static void Demo_SpawnGetsuga(void)
{
  if (s_getsugaActive != 0U)
  {
    return;
  }

  s_getsugaX = GETSUGA_START_X;
  s_getsugaY = GETSUGA_START_Y;
  s_getsugaFrameIndex = 0U;
  s_getsugaActive = 1U;

  SpriteRender_Draw(s_getsugaX,
                    s_getsugaY,
                    getsuga_projectile_frames[s_getsugaFrameIndex],
                    GETSUGA_PROJECTILE_WIDTH,
                    GETSUGA_PROJECTILE_HEIGHT,
                    0U);
}

static void Demo_UpdateGetsuga(void)
{
  if (s_getsugaActive == 0U)
  {
    return;
  }

  SpriteRender_Erase(s_getsugaX,
                     s_getsugaY,
                     getsuga_projectile_frames[s_getsugaFrameIndex],
                     GETSUGA_PROJECTILE_WIDTH,
                     GETSUGA_PROJECTILE_HEIGHT,
                     0U,
                     Demo_GetBackgroundPixel);

  s_getsugaX = (int16_t)(s_getsugaX + GETSUGA_SPEED_X);
  s_getsugaFrameIndex = (uint8_t)((s_frameCounter / GETSUGA_TICKS_PER_FRAME) % GETSUGA_PROJECTILE_FRAME_COUNT);

  if (s_getsugaX > (int16_t)LCD_PORT_WIDTH)
  {
    s_getsugaActive = 0U;
    return;
  }

  SpriteRender_Draw(s_getsugaX,
                    s_getsugaY,
                    getsuga_projectile_frames[s_getsugaFrameIndex],
                    GETSUGA_PROJECTILE_WIDTH,
                    GETSUGA_PROJECTILE_HEIGHT,
                    0U);

  Demo_DrawIchigo(s_ichigoSkillFrameIndex);
  Demo_DrawNaruto(s_narutoFrameIndex);
}

static uint16_t Demo_GetBackgroundPixel(uint16_t x, uint16_t y)
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
    uint16_t mountainX = (uint16_t)((x / 8U) * 8U);
    uint16_t localX = (uint16_t)(x - mountainX);
    uint16_t h1 = (uint16_t)(22U + ((mountainX * 5U) % 38U));
    uint16_t h2 = (uint16_t)(14U + ((mountainX * 7U) % 28U));

    if ((localX < 4U) && (y >= (uint16_t)(148U - h2)))
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

    if ((y < 150U) || ((x % 32U) < 2U))
    {
      color = RGB565_WALL_LIGHT;
    }

    if (y >= 180U)
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
  }

  return color;
}

static void Demo_DrawStaticSky(void)
{
  LCD_Port_FillRect(0U, 0U, LCD_PORT_WIDTH, 48U, RGB565_SKY_TOP);
  LCD_Port_FillRect(0U, 48U, LCD_PORT_WIDTH, 48U, RGB565_SKY_MID);
  LCD_Port_FillRect(0U, 96U, LCD_PORT_WIDTH, 52U, RGB565_SKY_LOW);
}

static void Demo_DrawSun(uint16_t x, uint16_t y)
{
  Demo_FillSafe((int16_t)x - 10, (int16_t)y - 10, 20, 20, RGB565_SUN);
  Demo_FillSafe((int16_t)x - 14, (int16_t)y - 4, 28, 8, RGB565_SUN);
  Demo_FillSafe((int16_t)x - 4, (int16_t)y - 14, 8, 28, RGB565_SUN);
  Demo_FillSafe((int16_t)x - 18, (int16_t)y, 5, 2, RGB565_SUN);
  Demo_FillSafe((int16_t)x + 13, (int16_t)y, 5, 2, RGB565_SUN);
}

static void Demo_DrawCloud(int16_t x, uint16_t y)
{
  Demo_FillSafe(x + 8,  (int16_t)y + 7, 45, 9, RGB565_CLOUD_SHADOW);
  Demo_FillSafe(x + 0,  (int16_t)y + 8, 24, 9, RGB565_CLOUD);
  Demo_FillSafe(x + 13, (int16_t)y + 2, 27, 15, RGB565_CLOUD);
  Demo_FillSafe(x + 34, (int16_t)y + 7, 27, 10, RGB565_CLOUD);
  Demo_FillSafe(x + 7,  (int16_t)y + 15, 48, 5, RGB565_CLOUD);
}

static void Demo_DrawMountains(void)
{
  for (uint16_t x = 0U; x < LCD_PORT_WIDTH; x += 8U)
  {
    uint16_t h1 = (uint16_t)(22U + ((x * 5U) % 38U));
    uint16_t h2 = (uint16_t)(14U + ((x * 7U) % 28U));

    Demo_FillSafe((int16_t)x, (int16_t)(148U - h1), 8, (int16_t)h1, RGB565_MOUNTAIN_DARK);
    Demo_FillSafe((int16_t)x, (int16_t)(148U - h2), 4, (int16_t)h2, RGB565_MOUNTAIN_LIGHT);
  }
}

static void Demo_DrawDojoWall(void)
{
  LCD_Port_FillRect(0U, 148U, LCD_PORT_WIDTH, 34U, RGB565_WALL_DARK);

  for (uint16_t x = 0U; x < LCD_PORT_WIDTH; x += 32U)
  {
    LCD_Port_FillRect(x, 148U, 2U, 34U, RGB565_WALL_LIGHT);
  }

  LCD_Port_FillRect(0U, 148U, LCD_PORT_WIDTH, 2U, RGB565_WALL_LIGHT);
  LCD_Port_FillRect(0U, 180U, LCD_PORT_WIDTH, 2U, RGB565_BLACK);

  Demo_FillSafe(132, 154, 56, 20, RGB565_GROUND_DARK);
  Demo_FillSafe(137, 158, 46, 12, RGB565_ACCENT_ORANGE);
  Demo_FillSafe(143, 162, 34, 4, RGB565_BLACK);
}

static void Demo_DrawGround(uint32_t frame)
{
  LCD_Port_FillRect(0U, 182U, LCD_PORT_WIDTH, 58U, RGB565_GROUND);
  LCD_Port_FillRect(0U, 182U, LCD_PORT_WIDTH, 6U, RGB565_GROUND_TOP);
  LCD_Port_FillRect(0U, 232U, LCD_PORT_WIDTH, 8U, RGB565_GROUND_DARK);

  for (uint16_t x = 0U; x < LCD_PORT_WIDTH; x += 24U)
  {
    uint16_t shade = (((x / 24U) + (frame / 8U)) & 1U) ? RGB565_GROUND_DARK : RGB565_WALL_LIGHT;
    Demo_FillSafe((int16_t)x, 198, 16, 2, shade);
    Demo_FillSafe((int16_t)(x + 10U), 218, 18, 2, shade);
  }
}

static void Demo_DrawArenaMarks(uint32_t frame)
{
  LCD_Port_FillRect(40U, 190U, 240U, 2U, RGB565_LINE);
  LCD_Port_FillRect(158U, 186U, 4U, 42U, RGB565_LINE);

  Demo_FillSafe(58, 199, 42, 4, RGB565_ACCENT_CYAN);
  Demo_FillSafe(220, 199, 42, 4, RGB565_ACCENT_ORANGE);

  uint16_t pulseX = (uint16_t)(152U + ((frame % 16U) / 2U));
  Demo_FillSafe((int16_t)pulseX, 205, 16, 3, RGB565_WHITE);
}

static void Demo_FillSafe(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color)
{
  if ((width <= 0) || (height <= 0))
  {
    return;
  }

  int16_t x2 = (int16_t)(x + width);
  int16_t y2 = (int16_t)(y + height);

  if ((x >= (int16_t)LCD_PORT_WIDTH) || (y >= (int16_t)LCD_PORT_HEIGHT) || (x2 <= 0) || (y2 <= 0))
  {
    return;
  }

  if (x < 0)
  {
    width = (int16_t)(width + x);
    x = 0;
  }

  if (y < 0)
  {
    height = (int16_t)(height + y);
    y = 0;
  }

  if ((x + width) > (int16_t)LCD_PORT_WIDTH)
  {
    width = (int16_t)((int16_t)LCD_PORT_WIDTH - x);
  }

  if ((y + height) > (int16_t)LCD_PORT_HEIGHT)
  {
    height = (int16_t)((int16_t)LCD_PORT_HEIGHT - y);
  }

  LCD_Port_FillRect((uint16_t)x, (uint16_t)y, (uint16_t)width, (uint16_t)height, color);
}
