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
#include "stm32f4xx_hal.h"

#define GAME_FPS               30U
#define FRAME_TIME_MS          (1000U / GAME_FPS)
#define BACKGROUND_ANIMATION_ENABLED  0U

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

static void Demo_DrawBackground(uint32_t frame);
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
  Demo_DrawBackground(s_frameCounter);
  LCD_Port_Flush();
}

void GameBackgroundDemo_Update(void)
{
#if BACKGROUND_ANIMATION_ENABLED
  uint32_t now = HAL_GetTick();

  if ((now - s_lastFrameMs) < FRAME_TIME_MS)
  {
    return;
  }

  s_lastFrameMs += FRAME_TIME_MS;
  s_frameCounter++;

  Demo_DrawBackground(s_frameCounter);
  LCD_Port_Flush();
#endif
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
