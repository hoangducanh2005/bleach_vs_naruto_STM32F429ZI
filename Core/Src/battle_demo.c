#include "battle_demo.h"

#include "combat_actor.h"
#include "combat_box.h"
#include "combat_input.h"
#include "chidori_data.h"
#include "ILI9341_GFX.h"
#include "lcd_port.h"
#include "sprite_data.h"
#include "sprite_render.h"
#include "stm32f4xx_hal.h"

#define BATTLE_TICK_MS 33U
#define BATTLE_GROUND_Y 205
#define BATTLE_P1_START_X 92
#define BATTLE_CPU_START_X 228
#define BATTLE_DEBUG_BOXES 0
#define BATTLE_GETSUGA_SPEED 10
#define BATTLE_GETSUGA_FRAME_MS 40U
#define BATTLE_GETSUGA_SPAWN_FRAME 4U

#define RGB565_BLACK 0x0000U
#define RGB565_WHITE 0xFFFFU
#define RGB565_SKY_TOP 0x5D7FU
#define RGB565_SKY_MID 0x865FU
#define RGB565_SKY_LOW 0xB73FU
#define RGB565_CLOUD 0xF7BEU
#define RGB565_CLOUD_SHADOW 0xCE79U
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
#define RGB565_HP_BACK 0x4208U
#define RGB565_HP_GOOD 0x07E0U
#define RGB565_HP_WARN 0xFD20U
#define RGB565_HURTBOX 0x07E0U
#define RGB565_HITBOX 0xF800U

typedef struct
{
  const uint16_t *pixels;
  uint16_t width;
  uint16_t height;
  int16_t x;
  int16_t y;
  uint8_t flipX;
  uint8_t valid;
} BattleActorSnapshot;

typedef struct
{
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;
  uint8_t valid;
} BattleDirtyRect;

typedef struct
{
  int16_t x;
  int16_t y;
  int16_t vx;
  uint8_t active;
  uint8_t frameIndex;
  uint8_t hitConnected;
  uint32_t startedMs;
} BattleProjectile;

static CombatActor s_player;
static CombatActor s_cpu;
static BattleProjectile s_getsuga;
static BattleActorSnapshot s_playerSnapshot;
static BattleActorSnapshot s_cpuSnapshot;
static BattleActorSnapshot s_getsugaSnapshot;
static BattleActorSnapshot s_chidoriSnapshot;
static uint16_t s_compositeRun[LCD_PORT_WIDTH];
static uint16_t s_backgroundPixels[LCD_PORT_WIDTH * LCD_PORT_HEIGHT];
static uint32_t s_lastTickMs;
static uint16_t s_lastPlayerHp;
static uint16_t s_lastCpuHp;
static uint32_t s_lastGetsugaSkillStartMs;

static const CombatHitboxDef s_getsugaHitbox = {
    0U,
    0U,
    {8, 18, 38, 36},
    15U,
    3U,
    300U,
    160U,
    22,
    0,
    COMBAT_HIT_LEVEL_PROJECTILE,
    1U,
};

static void Battle_DrawFrame(void);
static void Battle_DrawActorsInitial(void);
static void Battle_UpdateActors(void);
static void Battle_TrySpawnGetsuga(uint32_t nowMs);
static void Battle_UpdateGetsuga(uint32_t nowMs);
static void Battle_ResolveProjectileHit(BattleProjectile *projectile,
                                        CombatActor *owner,
                                        CombatActor *target,
                                        uint32_t nowMs);
static void Battle_DrawBackground(void);
static void Battle_BuildBackgroundCache(void);
static void Battle_DrawStaticSky(void);
static void Battle_DrawSun(uint16_t x, uint16_t y);
static void Battle_DrawCloud(int16_t x, uint16_t y);
static void Battle_DrawMountains(void);
static void Battle_DrawDojoWall(void);
static void Battle_DrawGround(void);
static void Battle_DrawArenaMarks(void);
static void Battle_DrawHud(void);
static void Battle_DrawHealthBar(uint16_t x, uint16_t y, uint16_t hp,
                                 uint16_t borderColor);
static void Battle_DrawActor(const CombatActor *actor,
                             BattleActorSnapshot *snapshot);
static uint8_t Battle_CaptureActor(const CombatActor *actor,
                                   BattleActorSnapshot *snapshot);
static uint8_t Battle_CaptureGetsuga(const BattleProjectile *projectile,
                                     BattleActorSnapshot *snapshot);
static uint8_t Battle_CaptureChidori(const CombatActor *actor,
                                     BattleActorSnapshot *snapshot);
static void Battle_DrawActorSnapshot(const BattleActorSnapshot *snapshot);
static uint8_t Battle_ActorSnapshotEqual(const BattleActorSnapshot *a,
                                         const BattleActorSnapshot *b);
static void Battle_DirtyRectAddSnapshot(BattleDirtyRect *rect,
                                        const BattleActorSnapshot *snapshot);
static void Battle_DrawSnapshotChange(const BattleActorSnapshot *previous,
                                      const BattleActorSnapshot *next,
                                      const BattleActorSnapshot *player,
                                      const BattleActorSnapshot *cpu,
                                      const BattleActorSnapshot *projectile,
                                      const BattleActorSnapshot *chidori);
static void Battle_DrawDirtyRect(BattleDirtyRect rect,
                                 const BattleActorSnapshot *player,
                                 const BattleActorSnapshot *cpu,
                                 const BattleActorSnapshot *projectile,
                                 const BattleActorSnapshot *chidori);
static void Battle_ComposeActorRow(BattleDirtyRect rect,
                                   const BattleActorSnapshot *snapshot,
                                   uint16_t screenY);
static void Battle_DrawDebugBoxes(void);
#if BATTLE_DEBUG_BOXES
static void Battle_DrawBox(CombatBox box, uint16_t color);
#endif
static void Battle_ResolveHit(CombatActor *attacker, CombatActor *target,
                              uint32_t nowMs);
static uint16_t Battle_GetBackgroundPixel(uint16_t x, uint16_t y);
static uint8_t Battle_RectContains(uint16_t x, uint16_t y, int16_t rectX,
                                   int16_t rectY, int16_t rectW,
                                   int16_t rectH);
static void Battle_FillSafe(int16_t x, int16_t y, int16_t width,
                            int16_t height, uint16_t color);

void BattleDemo_Init(void)
{
  uint32_t now = HAL_GetTick();

  LCD_Port_Init();
  CombatInput_Init();
  CombatActor_Init(&s_player,
                   COMBAT_CHARACTER_NARUTO,
                   BATTLE_P1_START_X,
                   BATTLE_GROUND_Y,
                   1,
                   now);
  CombatActor_Init(&s_cpu,
                   COMBAT_CHARACTER_SASUKE,
                   BATTLE_CPU_START_X,
                   BATTLE_GROUND_Y,
                   -1,
                   now);
  s_lastTickMs = now;
  s_lastPlayerHp = s_player.hp;
  s_lastCpuHp = s_cpu.hp;
  s_playerSnapshot.valid = 0U;
  s_cpuSnapshot.valid = 0U;
  s_getsugaSnapshot.valid = 0U;
  s_chidoriSnapshot.valid = 0U;
  s_getsuga.active = 0U;
  s_lastGetsugaSkillStartMs = 0U;

  Battle_BuildBackgroundCache();
  Battle_DrawFrame();
  LCD_Port_Flush();
}

void BattleDemo_Update(void)
{
  uint32_t now = HAL_GetTick();

  if ((now - s_lastTickMs) < BATTLE_TICK_MS)
  {
    return;
  }

  s_lastTickMs += BATTLE_TICK_MS;

  uint8_t input = CombatInput_Read();

  CombatActor_FaceToward(&s_player, &s_cpu);
  CombatActor_FaceToward(&s_cpu, &s_player);
  CombatActor_Update(&s_player, input, now, 1U);
  CombatActor_Update(&s_cpu, COMBAT_INPUT_NONE, now, 0U);
  Battle_ResolveHit(&s_player, &s_cpu, now);
  Battle_TrySpawnGetsuga(now);
  Battle_UpdateGetsuga(now);
  Battle_ResolveProjectileHit(&s_getsuga, &s_player, &s_cpu, now);

  if (s_player.hp != s_lastPlayerHp)
  {
    Battle_DrawHealthBar(12U, 22U, s_player.hp, RGB565_ACCENT_CYAN);
    s_lastPlayerHp = s_player.hp;
  }

  if (s_cpu.hp != s_lastCpuHp)
  {
    Battle_DrawHealthBar(202U, 22U, s_cpu.hp, RGB565_ACCENT_ORANGE);
    s_lastCpuHp = s_cpu.hp;
  }

  Battle_UpdateActors();
  LCD_Port_Flush();
}

static void Battle_ResolveHit(CombatActor *attacker, CombatActor *target,
                              uint32_t nowMs)
{
  const CombatHitboxDef *hitbox = 0;
  CombatBox hitboxWorld = CombatActor_GetHitboxWorld(attacker, &hitbox);
  CombatBox hurtboxWorld = CombatActor_GetHurtboxWorld(target);

  if ((hitbox == 0) || (attacker->hitConnected != 0U))
  {
    return;
  }

  if (CombatBox_Overlap(hitboxWorld, hurtboxWorld) == 0U)
  {
    return;
  }

  uint8_t blocked = CombatActor_IsBlockingFront(target, attacker);
  CombatActor_ApplyHit(target, attacker, hitbox, nowMs, blocked);

  if (hitbox->oneHitOnly != 0U)
  {
    attacker->hitConnected = 1U;
  }
}

static void Battle_DrawFrame(void)
{
  Battle_DrawBackground();
  Battle_DrawHud();
  Battle_DrawActorsInitial();
  Battle_DrawDebugBoxes();
}

static void Battle_DrawActorsInitial(void)
{
  Battle_DrawActor(&s_player, &s_playerSnapshot);
  Battle_DrawActor(&s_cpu, &s_cpuSnapshot);
}

static void Battle_UpdateActors(void)
{
  BattleActorSnapshot nextPlayer;
  BattleActorSnapshot nextCpu;
  BattleActorSnapshot nextGetsuga;
  BattleActorSnapshot nextChidori;
  uint8_t playerDirty;
  uint8_t cpuDirty;
  uint8_t getsugaDirty;
  uint8_t chidoriDirty;

  Battle_CaptureActor(&s_player, &nextPlayer);
  Battle_CaptureActor(&s_cpu, &nextCpu);
  Battle_CaptureGetsuga(&s_getsuga, &nextGetsuga);
  Battle_CaptureChidori(&s_player, &nextChidori);

  playerDirty = (Battle_ActorSnapshotEqual(&s_playerSnapshot, &nextPlayer) == 0U);
  cpuDirty = (Battle_ActorSnapshotEqual(&s_cpuSnapshot, &nextCpu) == 0U);
  getsugaDirty = (Battle_ActorSnapshotEqual(&s_getsugaSnapshot, &nextGetsuga) == 0U);
  chidoriDirty = (Battle_ActorSnapshotEqual(&s_chidoriSnapshot, &nextChidori) == 0U);

  if ((playerDirty == 0U) && (cpuDirty == 0U) &&
      (getsugaDirty == 0U) && (chidoriDirty == 0U))
  {
    return;
  }

  if (playerDirty != 0U)
  {
    Battle_DrawSnapshotChange(&s_playerSnapshot,
                              &nextPlayer,
                              &nextPlayer,
                              &nextCpu,
                              &nextGetsuga,
                              &nextChidori);
  }

  if (cpuDirty != 0U)
  {
    Battle_DrawSnapshotChange(&s_cpuSnapshot,
                              &nextCpu,
                              &nextPlayer,
                              &nextCpu,
                              &nextGetsuga,
                              &nextChidori);
  }

  if (getsugaDirty != 0U)
  {
    Battle_DrawSnapshotChange(&s_getsugaSnapshot,
                              &nextGetsuga,
                              &nextPlayer,
                              &nextCpu,
                              &nextGetsuga,
                              &nextChidori);
  }

  if (chidoriDirty != 0U)
  {
    Battle_DrawSnapshotChange(&s_chidoriSnapshot,
                              &nextChidori,
                              &nextPlayer,
                              &nextCpu,
                              &nextGetsuga,
                              &nextChidori);
  }

  s_playerSnapshot = nextPlayer;
  s_cpuSnapshot = nextCpu;
  s_getsugaSnapshot = nextGetsuga;
  s_chidoriSnapshot = nextChidori;
}

static void Battle_DrawActor(const CombatActor *actor,
                             BattleActorSnapshot *snapshot)
{
  if (Battle_CaptureActor(actor, snapshot) == 0U)
  {
    return;
  }

  Battle_DrawActorSnapshot(snapshot);
}

static uint8_t Battle_CaptureActor(const CombatActor *actor,
                                   BattleActorSnapshot *snapshot)
{
  CombatFrameView frame;
  CombatActor viewActor;

  if (snapshot != 0)
  {
    snapshot->valid = 0U;
  }

  if ((actor == 0) || (snapshot == 0))
  {
    return 0U;
  }

  viewActor = *actor;

  if (CombatActor_GetFrame(&viewActor, &frame) == 0U)
  {
    return 0U;
  }

  snapshot->pixels = frame.pixels;
  snapshot->width = frame.width;
  snapshot->height = frame.height;
  snapshot->x = (int16_t)(actor->x - frame.pivotX);
  snapshot->y = (int16_t)(actor->y - frame.pivotY);
  snapshot->flipX = (actor->facing < 0) ? 1U : 0U;
  snapshot->valid = 1U;

  return 1U;
}

static void Battle_TrySpawnGetsuga(uint32_t nowMs)
{
  if ((s_player.character != COMBAT_CHARACTER_ICHIGO) ||
      (s_player.state != COMBAT_ANIM_SKILL) ||
      (s_player.frameIndex < BATTLE_GETSUGA_SPAWN_FRAME) ||
      (s_lastGetsugaSkillStartMs == s_player.stateStartedMs))
  {
    return;
  }

  s_lastGetsugaSkillStartMs = s_player.stateStartedMs;
  s_getsuga.active = 1U;
  s_getsuga.frameIndex = 0U;
  s_getsuga.hitConnected = 0U;
  s_getsuga.startedMs = nowMs;
  s_getsuga.vx = (s_player.facing < 0) ? -BATTLE_GETSUGA_SPEED
                                       : BATTLE_GETSUGA_SPEED;
  s_getsuga.y = (int16_t)(s_player.y - 92);

  if (s_player.facing < 0)
  {
    s_getsuga.x = (int16_t)(s_player.x - 18 - (int16_t)GETSUGA_PROJECTILE_WIDTH);
  }
  else
  {
    s_getsuga.x = (int16_t)(s_player.x + 18);
  }
}

static void Battle_UpdateGetsuga(uint32_t nowMs)
{
  if (s_getsuga.active == 0U)
  {
    return;
  }

  s_getsuga.x = (int16_t)(s_getsuga.x + s_getsuga.vx);
  s_getsuga.frameIndex = (uint8_t)(((nowMs - s_getsuga.startedMs) /
                                    BATTLE_GETSUGA_FRAME_MS) %
                                   GETSUGA_PROJECTILE_FRAME_COUNT);

  if ((s_getsuga.x > (int16_t)LCD_PORT_WIDTH) ||
      ((s_getsuga.x + (int16_t)GETSUGA_PROJECTILE_WIDTH) < 0))
  {
    s_getsuga.active = 0U;
  }
}

static void Battle_ResolveProjectileHit(BattleProjectile *projectile,
                                        CombatActor *owner,
                                        CombatActor *target,
                                        uint32_t nowMs)
{
  if ((projectile == 0) || (owner == 0) || (target == 0) ||
      (projectile->active == 0U) ||
      (projectile->hitConnected != 0U))
  {
    return;
  }

  CombatBox projectileBox = {
      (int16_t)(projectile->x + s_getsugaHitbox.box.x),
      (int16_t)(projectile->y + s_getsugaHitbox.box.y),
      s_getsugaHitbox.box.w,
      s_getsugaHitbox.box.h,
  };
  CombatBox hurtboxWorld = CombatActor_GetHurtboxWorld(target);

  if (CombatBox_Overlap(projectileBox, hurtboxWorld) == 0U)
  {
    return;
  }

  CombatActor_ApplyHit(target, owner, &s_getsugaHitbox, nowMs, 0U);
  projectile->hitConnected = 1U;
  projectile->active = 0U;
}

static uint8_t Battle_CaptureGetsuga(const BattleProjectile *projectile,
                                     BattleActorSnapshot *snapshot)
{
  if (snapshot != 0)
  {
    snapshot->valid = 0U;
  }

  if ((projectile == 0) || (snapshot == 0) ||
      (projectile->active == 0U))
  {
    return 0U;
  }

  snapshot->pixels = getsuga_projectile_frames[projectile->frameIndex];
  snapshot->width = GETSUGA_PROJECTILE_WIDTH;
  snapshot->height = GETSUGA_PROJECTILE_HEIGHT;
  snapshot->x = projectile->x;
  snapshot->y = projectile->y;
  snapshot->flipX = (projectile->vx < 0) ? 1U : 0U;
  snapshot->valid = 1U;

  return 1U;
}

static uint8_t Battle_CaptureChidori(const CombatActor *actor,
                                     BattleActorSnapshot *snapshot)
{
  const ChidoriFrame *frames = 0;
  uint8_t frameCount = 0U;
  uint8_t frameIndex = 0U;

  if (snapshot != 0)
  {
    snapshot->valid = 0U;
  }

  if ((actor == 0) || (snapshot == 0) ||
      (actor->character != COMBAT_CHARACTER_SASUKE) ||
      (actor->state != COMBAT_ANIM_SKILL))
  {
    return 0U;
  }

  if (actor->frameIndex < 3U)
  {
    frames = chidori_1_frames;
    frameCount = 4U;
    frameIndex = actor->frameIndex;
  }
  else if (actor->frameIndex < 6U)
  {
    frames = chidori_2_frames;
    frameCount = 3U;
    frameIndex = (uint8_t)(actor->frameIndex - 3U);
  }
  else if (actor->frameIndex < 9U)
  {
    frames = chidori_3_frames;
    frameCount = 3U;
    frameIndex = (uint8_t)(actor->frameIndex - 6U);
  }
  else
  {
    frames = chidori_4_frames;
    frameCount = 2U;
    frameIndex = (uint8_t)(actor->frameIndex - 9U);
  }

  if (frameIndex >= frameCount)
  {
    frameIndex = (uint8_t)(frameCount - 1U);
  }

  const ChidoriFrame *frame = &frames[frameIndex];
  snapshot->pixels = frame->pixels;
  snapshot->width = frame->width;
  snapshot->height = frame->height;
  snapshot->y = (int16_t)(actor->y - 54 - (int16_t)(frame->height / 2U));

  if (actor->facing < 0)
  {
    snapshot->x = (int16_t)(actor->x - 18 - (int16_t)frame->width);
    snapshot->flipX = 1U;
  }
  else
  {
    snapshot->x = (int16_t)(actor->x + 18);
    snapshot->flipX = 0U;
  }

  snapshot->valid = 1U;
  return 1U;
}

static void Battle_DrawActorSnapshot(const BattleActorSnapshot *snapshot)
{
  if ((snapshot == 0) || (snapshot->valid == 0U))
  {
    return;
  }

  SpriteRender_Draw(snapshot->x,
                    snapshot->y,
                    snapshot->pixels,
                    snapshot->width,
                    snapshot->height,
                    snapshot->flipX);
}

static uint8_t Battle_ActorSnapshotEqual(const BattleActorSnapshot *a,
                                         const BattleActorSnapshot *b)
{
  if ((a == 0) || (b == 0))
  {
    return 0U;
  }

  return ((a->valid == b->valid) &&
          (a->pixels == b->pixels) &&
          (a->width == b->width) &&
          (a->height == b->height) &&
          (a->x == b->x) &&
          (a->y == b->y) &&
          (a->flipX == b->flipX))
             ? 1U
             : 0U;
}

static void Battle_DirtyRectAddSnapshot(BattleDirtyRect *rect,
                                        const BattleActorSnapshot *snapshot)
{
  if ((rect == 0) || (snapshot == 0) || (snapshot->valid == 0U) ||
      (snapshot->width == 0U) || (snapshot->height == 0U))
  {
    return;
  }

  int16_t x0 = snapshot->x;
  int16_t y0 = snapshot->y;
  int16_t x1 = (int16_t)(snapshot->x + (int16_t)snapshot->width);
  int16_t y1 = (int16_t)(snapshot->y + (int16_t)snapshot->height);

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

  if (rect->valid == 0U)
  {
    rect->x = x0;
    rect->y = y0;
    rect->w = (int16_t)(x1 - x0);
    rect->h = (int16_t)(y1 - y0);
    rect->valid = 1U;
    return;
  }

  int16_t rx1 = (int16_t)(rect->x + rect->w);
  int16_t ry1 = (int16_t)(rect->y + rect->h);

  if (x0 < rect->x)
  {
    rect->x = x0;
  }
  if (y0 < rect->y)
  {
    rect->y = y0;
  }
  if (x1 > rx1)
  {
    rx1 = x1;
  }
  if (y1 > ry1)
  {
    ry1 = y1;
  }

  rect->w = (int16_t)(rx1 - rect->x);
  rect->h = (int16_t)(ry1 - rect->y);
}

static void Battle_DrawSnapshotChange(const BattleActorSnapshot *previous,
                                      const BattleActorSnapshot *next,
                                      const BattleActorSnapshot *player,
                                      const BattleActorSnapshot *cpu,
                                      const BattleActorSnapshot *projectile,
                                      const BattleActorSnapshot *chidori)
{
  BattleDirtyRect dirty = {0, 0, 0, 0, 0U};

  Battle_DirtyRectAddSnapshot(&dirty, previous);
  Battle_DirtyRectAddSnapshot(&dirty, next);
  Battle_DrawDirtyRect(dirty, player, cpu, projectile, chidori);
}

static void Battle_DrawDirtyRect(BattleDirtyRect rect,
                                 const BattleActorSnapshot *player,
                                 const BattleActorSnapshot *cpu,
                                 const BattleActorSnapshot *projectile,
                                 const BattleActorSnapshot *chidori)
{
  if ((rect.valid == 0U) || (rect.w <= 0) || (rect.h <= 0))
  {
    return;
  }

  for (int16_t row = 0; row < rect.h; row++)
  {
    uint16_t y = (uint16_t)(rect.y + row);

    for (int16_t col = 0; col < rect.w; col++)
    {
      uint16_t x = (uint16_t)(rect.x + col);
      s_compositeRun[col] =
          s_backgroundPixels[((uint32_t)y * LCD_PORT_WIDTH) + x];
    }

    Battle_ComposeActorRow(rect, player, y);
    Battle_ComposeActorRow(rect, cpu, y);
    Battle_ComposeActorRow(rect, projectile, y);
    Battle_ComposeActorRow(rect, chidori, y);

    LCD_Port_DrawPixels((uint16_t)rect.x,
                        y,
                        (uint16_t)rect.w,
                        s_compositeRun);
  }
}

static void Battle_ComposeActorRow(BattleDirtyRect rect,
                                   const BattleActorSnapshot *snapshot,
                                   uint16_t screenY)
{
  int16_t y = (int16_t)screenY;

  if ((snapshot == 0) || (snapshot->valid == 0U) ||
      (y < snapshot->y) ||
      (y >= (int16_t)(snapshot->y + (int16_t)snapshot->height)))
  {
    return;
  }

  int16_t x0 = snapshot->x;
  int16_t x1 = (int16_t)(snapshot->x + (int16_t)snapshot->width);
  int16_t rectX1 = (int16_t)(rect.x + rect.w);

  if (x0 < rect.x)
  {
    x0 = rect.x;
  }
  if (x1 > rectX1)
  {
    x1 = rectX1;
  }

  if (x1 <= x0)
  {
    return;
  }

  uint16_t localY = (uint16_t)(y - snapshot->y);

  for (int16_t screenX = x0; screenX < x1; screenX++)
  {
    uint16_t localX = (uint16_t)(screenX - snapshot->x);
    uint16_t srcX = (snapshot->flipX != 0U)
                        ? (uint16_t)(snapshot->width - 1U - localX)
                        : localX;
    uint16_t color = snapshot->pixels[((uint32_t)localY * snapshot->width) + srcX];

    if (color != SPRITE_COLOR_KEY_RGB565)
    {
      s_compositeRun[screenX - rect.x] = color;
    }
  }
}

static void Battle_DrawDebugBoxes(void)
{
#if BATTLE_DEBUG_BOXES
  const CombatHitboxDef *unusedHitbox = 0;
  Battle_DrawBox(CombatActor_GetHurtboxWorld(&s_player), RGB565_HURTBOX);
  Battle_DrawBox(CombatActor_GetHurtboxWorld(&s_cpu), RGB565_HURTBOX);
  Battle_DrawBox(CombatActor_GetHitboxWorld(&s_player, &unusedHitbox),
                 RGB565_HITBOX);
  Battle_DrawBox(CombatActor_GetHitboxWorld(&s_cpu, &unusedHitbox),
                 RGB565_HITBOX);
#endif
}

#if BATTLE_DEBUG_BOXES
static void Battle_DrawBox(CombatBox box, uint16_t color)
{
  if ((box.w <= 0) || (box.h <= 0))
  {
    return;
  }

  int16_t x0 = box.x;
  int16_t y0 = box.y;
  int16_t x1 = (int16_t)(box.x + box.w);
  int16_t y1 = (int16_t)(box.y + box.h);

  if ((x1 <= 0) || (y1 <= 0) ||
      (x0 >= (int16_t)LCD_PORT_WIDTH) ||
      (y0 >= (int16_t)LCD_PORT_HEIGHT))
  {
    return;
  }

  x0 = CombatBox_ClampI16(x0, 0, (int16_t)(LCD_PORT_WIDTH - 1U));
  y0 = CombatBox_ClampI16(y0, 0, (int16_t)(LCD_PORT_HEIGHT - 1U));
  x1 = CombatBox_ClampI16(x1, 0, (int16_t)(LCD_PORT_WIDTH - 1U));
  y1 = CombatBox_ClampI16(y1, 0, (int16_t)(LCD_PORT_HEIGHT - 1U));

  ILI9341_DrawHollowRectangleCoord((uint16_t)x0,
                                   (uint16_t)y0,
                                   (uint16_t)x1,
                                   (uint16_t)y1,
                                   color);
}
#endif

static void Battle_DrawHud(void)
{
  ILI9341_DrawFilledRectangleCoord(6U, 6U, 314U, 35U, RGB565_BLACK);
  ILI9341_DrawText("P1 NARUTO", FONT2, 12U, 10U, RGB565_WHITE, RGB565_BLACK);
  ILI9341_DrawText("CPU SASUKE", FONT2, 214U, 10U, RGB565_WHITE, RGB565_BLACK);
  ILI9341_DrawText("BTN:ATK JMP SKL DSH", FONT2, 93U, 24U, RGB565_LINE,
                   RGB565_BLACK);
  Battle_DrawHealthBar(12U, 22U, s_player.hp, RGB565_ACCENT_CYAN);
  Battle_DrawHealthBar(202U, 22U, s_cpu.hp, RGB565_ACCENT_ORANGE);
}

static void Battle_DrawHealthBar(uint16_t x, uint16_t y, uint16_t hp,
                                 uint16_t borderColor)
{
  uint16_t fill = (hp > 30U) ? RGB565_HP_GOOD : RGB565_HP_WARN;
  uint16_t width = (uint16_t)((hp > 100U) ? 100U : hp);

  ILI9341_DrawHollowRectangleCoord(x, y, (uint16_t)(x + 102U),
                                   (uint16_t)(y + 8U), borderColor);
  LCD_Port_FillRect((uint16_t)(x + 1U), (uint16_t)(y + 1U), 100U, 6U,
                    RGB565_HP_BACK);
  if (width != 0U)
  {
    LCD_Port_FillRect((uint16_t)(x + 1U), (uint16_t)(y + 1U), width, 6U,
                      fill);
  }
}

static void Battle_DrawBackground(void)
{
  Battle_DrawStaticSky();
  Battle_DrawSun(268U, 34U);
  Battle_DrawCloud(30, 36U);
  Battle_DrawCloud(178, 58U);
  Battle_DrawMountains();
  Battle_DrawDojoWall();
  Battle_DrawGround();
  Battle_DrawArenaMarks();
}

static void Battle_BuildBackgroundCache(void)
{
  for (uint16_t y = 0U; y < LCD_PORT_HEIGHT; y++)
  {
    for (uint16_t x = 0U; x < LCD_PORT_WIDTH; x++)
    {
      s_backgroundPixels[((uint32_t)y * LCD_PORT_WIDTH) + x] =
          Battle_GetBackgroundPixel(x, y);
    }
  }
}

static void Battle_DrawStaticSky(void)
{
  LCD_Port_FillRect(0U, 0U, LCD_PORT_WIDTH, 48U, RGB565_SKY_TOP);
  LCD_Port_FillRect(0U, 48U, LCD_PORT_WIDTH, 48U, RGB565_SKY_MID);
  LCD_Port_FillRect(0U, 96U, LCD_PORT_WIDTH, 52U, RGB565_SKY_LOW);
}

static void Battle_DrawSun(uint16_t x, uint16_t y)
{
  Battle_FillSafe((int16_t)x - 10, (int16_t)y - 10, 20, 20,
                  RGB565_ACCENT_ORANGE);
  Battle_FillSafe((int16_t)x - 14, (int16_t)y - 4, 28, 8,
                  RGB565_ACCENT_ORANGE);
  Battle_FillSafe((int16_t)x - 4, (int16_t)y - 14, 8, 28,
                  RGB565_ACCENT_ORANGE);
}

static void Battle_DrawCloud(int16_t x, uint16_t y)
{
  Battle_FillSafe(x + 8, (int16_t)y + 7, 45, 9, RGB565_CLOUD_SHADOW);
  Battle_FillSafe(x, (int16_t)y + 8, 24, 9, RGB565_CLOUD);
  Battle_FillSafe(x + 13, (int16_t)y + 2, 27, 15, RGB565_CLOUD);
  Battle_FillSafe(x + 34, (int16_t)y + 7, 27, 10, RGB565_CLOUD);
  Battle_FillSafe(x + 7, (int16_t)y + 15, 48, 5, RGB565_CLOUD);
}

static void Battle_DrawMountains(void)
{
  for (uint16_t x = 0U; x < LCD_PORT_WIDTH; x += 8U)
  {
    uint16_t h1 = (uint16_t)(22U + ((x * 17U) % 39U));
    uint16_t h2 = (uint16_t)(14U + ((x * 11U) % 31U));
    Battle_FillSafe((int16_t)x, (int16_t)(148U - h1), 8, (int16_t)h1,
                    RGB565_MOUNTAIN_DARK);
    Battle_FillSafe((int16_t)x, (int16_t)(148U - h2), 4, (int16_t)h2,
                    RGB565_MOUNTAIN_LIGHT);
  }
}

static void Battle_DrawDojoWall(void)
{
  LCD_Port_FillRect(0U, 148U, LCD_PORT_WIDTH, 34U, RGB565_WALL_DARK);

  for (uint16_t x = 0U; x < LCD_PORT_WIDTH; x += 32U)
  {
    LCD_Port_FillRect(x, 148U, 2U, 34U, RGB565_WALL_LIGHT);
  }

  LCD_Port_FillRect(0U, 148U, LCD_PORT_WIDTH, 2U, RGB565_WALL_LIGHT);
  LCD_Port_FillRect(0U, 180U, LCD_PORT_WIDTH, 2U, RGB565_BLACK);
}

static void Battle_DrawGround(void)
{
  LCD_Port_FillRect(0U, 182U, LCD_PORT_WIDTH, 58U, RGB565_GROUND);
  LCD_Port_FillRect(0U, 182U, LCD_PORT_WIDTH, 6U, RGB565_GROUND_TOP);
  LCD_Port_FillRect(0U, 232U, LCD_PORT_WIDTH, 8U, RGB565_GROUND_DARK);
}

static void Battle_DrawArenaMarks(void)
{
  LCD_Port_FillRect(40U, 190U, 240U, 2U, RGB565_LINE);
  LCD_Port_FillRect(158U, 186U, 4U, 42U, RGB565_LINE);
  Battle_FillSafe(58, 199, 42, 4, RGB565_ACCENT_CYAN);
  Battle_FillSafe(220, 199, 42, 4, RGB565_ACCENT_ORANGE);
}

static uint16_t Battle_GetBackgroundPixel(uint16_t x, uint16_t y)
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
    uint16_t h1 = (uint16_t)(22U + ((mountainX * 17U) % 39U));
    uint16_t h2 = (uint16_t)(14U + ((mountainX * 11U) % 31U));

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

    if ((x % 32U) < 2U)
    {
      color = RGB565_WALL_LIGHT;
    }

    if ((y < 150U) || (y >= 180U))
    {
      color = (y < 150U) ? RGB565_WALL_LIGHT : RGB565_BLACK;
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

  if (Battle_RectContains(x, y, 258, 24, 20, 20) != 0U)
  {
    color = RGB565_ACCENT_ORANGE;
  }
  if (Battle_RectContains(x, y, 254, 30, 28, 8) != 0U)
  {
    color = RGB565_ACCENT_ORANGE;
  }
  if (Battle_RectContains(x, y, 264, 20, 8, 28) != 0U)
  {
    color = RGB565_ACCENT_ORANGE;
  }

  if (Battle_RectContains(x, y, 38, 43, 45, 9) != 0U)
  {
    color = RGB565_CLOUD_SHADOW;
  }
  if (Battle_RectContains(x, y, 30, 44, 24, 9) != 0U)
  {
    color = RGB565_CLOUD;
  }
  if (Battle_RectContains(x, y, 43, 38, 27, 15) != 0U)
  {
    color = RGB565_CLOUD;
  }
  if (Battle_RectContains(x, y, 64, 43, 27, 10) != 0U)
  {
    color = RGB565_CLOUD;
  }
  if (Battle_RectContains(x, y, 37, 51, 48, 5) != 0U)
  {
    color = RGB565_CLOUD;
  }

  if (Battle_RectContains(x, y, 186, 65, 45, 9) != 0U)
  {
    color = RGB565_CLOUD_SHADOW;
  }
  if (Battle_RectContains(x, y, 178, 66, 24, 9) != 0U)
  {
    color = RGB565_CLOUD;
  }
  if (Battle_RectContains(x, y, 191, 60, 27, 15) != 0U)
  {
    color = RGB565_CLOUD;
  }
  if (Battle_RectContains(x, y, 212, 65, 27, 10) != 0U)
  {
    color = RGB565_CLOUD;
  }
  if (Battle_RectContains(x, y, 185, 73, 48, 5) != 0U)
  {
    color = RGB565_CLOUD;
  }

  return color;
}

static uint8_t Battle_RectContains(uint16_t x, uint16_t y, int16_t rectX,
                                   int16_t rectY, int16_t rectW,
                                   int16_t rectH)
{
  return (((int16_t)x >= rectX) &&
          ((int16_t)x < (int16_t)(rectX + rectW)) &&
          ((int16_t)y >= rectY) &&
          ((int16_t)y < (int16_t)(rectY + rectH)))
             ? 1U
             : 0U;
}

static void Battle_FillSafe(int16_t x, int16_t y, int16_t width,
                            int16_t height, uint16_t color)
{
  if ((width <= 0) || (height <= 0))
  {
    return;
  }

  int16_t x2 = (int16_t)(x + width);
  int16_t y2 = (int16_t)(y + height);

  if ((x >= (int16_t)LCD_PORT_WIDTH) || (y >= (int16_t)LCD_PORT_HEIGHT) ||
      (x2 <= 0) || (y2 <= 0))
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

  LCD_Port_FillRect((uint16_t)x, (uint16_t)y, (uint16_t)width,
                    (uint16_t)height, color);
}
