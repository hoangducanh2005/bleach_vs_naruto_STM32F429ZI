/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : ichigo_animation_demo.c
  * @brief          : Ichigo moveset animation demo for the STM32 LCD.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "ichigo_animation_demo.h"

#include "ILI9341_GFX.h"
#include "ichigo_moveset.h"
#include "lcd_port.h"
#include "sprite_data.h"
#include "sprite_render.h"
#include "stm32f4xx_hal.h"

#define DEMO_FRAME_TIME_MS          33U
#define DEMO_STATE_HOLD_MS          450U
#define DEMO_GROUND_Y               205
#define DEMO_ANCHOR_X               92
#define DEMO_PROJECTILE_SPEED_X     18
#define DEMO_PROJECTILE_START_DX    34
#define DEMO_PROJECTILE_START_DY   -84
#define DEMO_SKILL_SPAWN_FRAME      5U

#define RGB565_BLACK                0x0000U
#define RGB565_WHITE                0xFFFFU
#define RGB565_SKY_TOP              0x5D7FU
#define RGB565_SKY_MID              0x865FU
#define RGB565_SKY_LOW              0xB73FU
#define RGB565_CLOUD                0xF7BEU
#define RGB565_CLOUD_SHADOW         0xCE79U
#define RGB565_MOUNTAIN_DARK        0x4A69U
#define RGB565_MOUNTAIN_LIGHT       0x6B6DU
#define RGB565_WALL_DARK            0x4208U
#define RGB565_WALL_LIGHT           0x632CU
#define RGB565_GROUND_TOP           0x7BEFU
#define RGB565_GROUND               0x39E7U
#define RGB565_GROUND_DARK          0x2104U
#define RGB565_LINE                 0xEF5DU
#define RGB565_ACCENT_ORANGE        0xFD20U
#define RGB565_ACCENT_CYAN          0x07FFU
#define RGB565_LABEL_BG             0x0008U

typedef struct
{
  IchigoMoveState state;
  const char *label;
} DemoStateStep;

static const DemoStateStep s_sequence[] = {
  { ICHIGO_MOVE_IDLE,         "IDLE" },
  { ICHIGO_MOVE_RUN,          "RUN" },
  { ICHIGO_MOVE_JUMP,         "JUMP" },
  { ICHIGO_MOVE_ATTACK_LIGHT, "ATTACK" },
  { ICHIGO_MOVE_SKILL,        "SKILL" },
  { ICHIGO_MOVE_HIT,          "HIT" },
  { ICHIGO_MOVE_DEAD,         "DEAD" }
};

static uint32_t s_lastFrameMs;
static uint32_t s_stateStartedMs;
static uint8_t s_sequenceIndex;
static uint8_t s_frameIndex;
static uint8_t s_previousFrameValid;
static const IchigoMoveFrame *s_previousFrame;
static int16_t s_previousX;
static int16_t s_previousY;
static uint8_t s_projectileActive;
static uint8_t s_projectileFrameIndex;
static int16_t s_projectileX;
static int16_t s_projectileY;

static void Demo_DrawBackground(void);
static void Demo_DrawStateLabel(const char *label);
static void Demo_SetState(uint8_t sequenceIndex);
static void Demo_DrawCurrentFrame(void);
static void Demo_ErasePreviousFrame(void);
static void Demo_UpdateProjectile(void);
static void Demo_SpawnProjectile(void);
static void Demo_EraseProjectile(void);
static uint16_t Demo_GetBackgroundPixel(uint16_t x, uint16_t y);
static void Demo_DrawStaticSky(void);
static void Demo_DrawSun(uint16_t x, uint16_t y);
static void Demo_DrawCloud(int16_t x, uint16_t y);
static void Demo_DrawMountains(void);
static void Demo_DrawDojoWall(void);
static void Demo_DrawGround(void);
static void Demo_DrawArenaMarks(void);
static void Demo_FillSafe(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color);

void IchigoAnimationDemo_Init(void)
{
  LCD_Port_Init();
  Demo_DrawBackground();

  s_lastFrameMs = HAL_GetTick();
  s_previousFrameValid = 0U;
  s_projectileActive = 0U;
  s_projectileFrameIndex = 0U;

  Demo_SetState(0U);
  Demo_DrawCurrentFrame();
  LCD_Port_Flush();
}

void IchigoAnimationDemo_Update(void)
{
  uint32_t now = HAL_GetTick();

  if ((now - s_lastFrameMs) < DEMO_FRAME_TIME_MS)
  {
    return;
  }

  s_lastFrameMs += DEMO_FRAME_TIME_MS;

  const IchigoMoveAnimation *animation = &ichigo_move_animations[s_sequence[s_sequenceIndex].state];
  uint16_t frameDuration = animation->frames[s_frameIndex].durationMs;

  if ((now - s_stateStartedMs) >= (((uint32_t)animation->frameCount * frameDuration) + DEMO_STATE_HOLD_MS))
  {
    uint8_t nextSequence = (uint8_t)(s_sequenceIndex + 1U);
    if (nextSequence >= (uint8_t)(sizeof(s_sequence) / sizeof(s_sequence[0])))
    {
      nextSequence = 0U;
    }

    Demo_ErasePreviousFrame();
    Demo_EraseProjectile();
    Demo_SetState(nextSequence);
    Demo_DrawCurrentFrame();
    LCD_Port_Flush();
    return;
  }

  uint32_t rawFrame = (now - s_stateStartedMs) / frameDuration;
  uint8_t nextFrame;

  if (animation->loop != 0U)
  {
    nextFrame = (uint8_t)(rawFrame % animation->frameCount);
  }
  else if (rawFrame >= animation->frameCount)
  {
    nextFrame = (uint8_t)(animation->frameCount - 1U);
  }
  else
  {
    nextFrame = (uint8_t)rawFrame;
  }

  if (nextFrame != s_frameIndex)
  {
    Demo_ErasePreviousFrame();
    s_frameIndex = nextFrame;
    Demo_DrawCurrentFrame();

    if ((s_sequence[s_sequenceIndex].state == ICHIGO_MOVE_SKILL) &&
        (s_frameIndex == DEMO_SKILL_SPAWN_FRAME))
    {
      Demo_SpawnProjectile();
    }
  }

  Demo_UpdateProjectile();
  LCD_Port_Flush();
}

static void Demo_SetState(uint8_t sequenceIndex)
{
  s_sequenceIndex = sequenceIndex;
  s_frameIndex = 0U;
  s_stateStartedMs = HAL_GetTick();
  s_previousFrameValid = 0U;
  Demo_DrawStateLabel(s_sequence[s_sequenceIndex].label);
}

static void Demo_DrawCurrentFrame(void)
{
  const IchigoMoveAnimation *animation = &ichigo_move_animations[s_sequence[s_sequenceIndex].state];
  const IchigoMoveFrame *frame = &animation->frames[s_frameIndex];
  int16_t x = (int16_t)(DEMO_ANCHOR_X - frame->pivotX);
  int16_t y = (int16_t)(DEMO_GROUND_Y - frame->pivotY);

  SpriteRender_Draw(x, y, frame->pixels, frame->width, frame->height, 0U);

  s_previousFrame = frame;
  s_previousX = x;
  s_previousY = y;
  s_previousFrameValid = 1U;
}

static void Demo_ErasePreviousFrame(void)
{
  if (s_previousFrameValid == 0U)
  {
    return;
  }

  SpriteRender_Erase(s_previousX,
                     s_previousY,
                     s_previousFrame->pixels,
                     s_previousFrame->width,
                     s_previousFrame->height,
                     0U,
                     Demo_GetBackgroundPixel);
  s_previousFrameValid = 0U;
}

static void Demo_SpawnProjectile(void)
{
  if (s_projectileActive != 0U)
  {
    return;
  }

  s_projectileActive = 1U;
  s_projectileFrameIndex = 0U;
  s_projectileX = (int16_t)(DEMO_ANCHOR_X + DEMO_PROJECTILE_START_DX);
  s_projectileY = (int16_t)(DEMO_GROUND_Y + DEMO_PROJECTILE_START_DY);

  SpriteRender_Draw(s_projectileX,
                    s_projectileY,
                    getsuga_projectile_frames[s_projectileFrameIndex],
                    GETSUGA_PROJECTILE_WIDTH,
                    GETSUGA_PROJECTILE_HEIGHT,
                    0U);
}

static void Demo_UpdateProjectile(void)
{
  if (s_projectileActive == 0U)
  {
    return;
  }

  Demo_EraseProjectile();

  s_projectileX = (int16_t)(s_projectileX + DEMO_PROJECTILE_SPEED_X);
  if (s_projectileX > (int16_t)LCD_PORT_WIDTH)
  {
    return;
  }

  s_projectileActive = 1U;
  s_projectileFrameIndex ^= 1U;
  SpriteRender_Draw(s_projectileX,
                    s_projectileY,
                    getsuga_projectile_frames[s_projectileFrameIndex],
                    GETSUGA_PROJECTILE_WIDTH,
                    GETSUGA_PROJECTILE_HEIGHT,
                    0U);
  Demo_DrawCurrentFrame();
}

static void Demo_EraseProjectile(void)
{
  if (s_projectileActive == 0U)
  {
    return;
  }

  SpriteRender_Erase(s_projectileX,
                     s_projectileY,
                     getsuga_projectile_frames[s_projectileFrameIndex],
                     GETSUGA_PROJECTILE_WIDTH,
                     GETSUGA_PROJECTILE_HEIGHT,
                     0U,
                     Demo_GetBackgroundPixel);
  s_projectileActive = 0U;
}

static void Demo_DrawBackground(void)
{
  Demo_DrawStaticSky();
  Demo_DrawSun(268U, 34U);
  Demo_DrawCloud(30, 34U);
  Demo_DrawCloud(178, 58U);
  Demo_DrawMountains();
  Demo_DrawDojoWall();
  Demo_DrawGround();
  Demo_DrawArenaMarks();
}

static void Demo_DrawStateLabel(const char *label)
{
  LCD_Port_FillRect(7U, 6U, 134U, 19U, RGB565_LABEL_BG);
  ILI9341_DrawText("ICHIGO:", FONT2, 12U, 9U, RGB565_ACCENT_CYAN, RGB565_LABEL_BG);
  ILI9341_DrawText(label, FONT2, 80U, 9U, RGB565_WHITE, RGB565_LABEL_BG);
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
  Demo_FillSafe((int16_t)x - 10, (int16_t)y - 10, 20, 20, RGB565_ACCENT_ORANGE);
  Demo_FillSafe((int16_t)x - 14, (int16_t)y - 4, 28, 8, RGB565_ACCENT_ORANGE);
  Demo_FillSafe((int16_t)x - 4, (int16_t)y - 14, 8, 28, RGB565_ACCENT_ORANGE);
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
}

static void Demo_DrawGround(void)
{
  LCD_Port_FillRect(0U, 182U, LCD_PORT_WIDTH, 58U, RGB565_GROUND);
  LCD_Port_FillRect(0U, 182U, LCD_PORT_WIDTH, 6U, RGB565_GROUND_TOP);
  LCD_Port_FillRect(0U, 232U, LCD_PORT_WIDTH, 8U, RGB565_GROUND_DARK);

  for (uint16_t x = 0U; x < LCD_PORT_WIDTH; x += 24U)
  {
    uint16_t shade = ((x / 24U) & 1U) ? RGB565_GROUND_DARK : RGB565_WALL_LIGHT;
    Demo_FillSafe((int16_t)x, 198, 16, 2, shade);
    Demo_FillSafe((int16_t)(x + 10U), 218, 18, 2, shade);
  }
}

static void Demo_DrawArenaMarks(void)
{
  LCD_Port_FillRect(40U, 190U, 240U, 2U, RGB565_LINE);
  LCD_Port_FillRect(158U, 186U, 4U, 42U, RGB565_LINE);
  Demo_FillSafe(58, 199, 42, 4, RGB565_ACCENT_CYAN);
  Demo_FillSafe(220, 199, 42, 4, RGB565_ACCENT_ORANGE);
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
