#include "battle_demo.h"

#if defined(__GNUC__)
#pragma GCC optimize ("O2")
#endif

#include "combat_actor.h"
#include "combat_box.h"
#include "combat_rules.h"
#include "combat_input.h"
#include "chidori_data.h"
#include "ILI9341_GFX.h"
#include "lcd_port.h"
#include "naruto_full_nine_tails_moveset.h"
#include "sprite_data.h"
#include "sprite_render.h"
#include "stm32f4xx_hal.h"
#include "vizard_moveset.h"
#include "buzzer.h"

#define BATTLE_TICK_MS 33U
#define BATTLE_GROUND_Y 205
#define BATTLE_P1_START_X 92
#define BATTLE_CPU_START_X 228
#define BATTLE_DEBUG_BOXES 0
#define BATTLE_GETSUGA_SPEED 10
#define BATTLE_GETSUGA_FRAME_MS 40U
#define BATTLE_GETSUGA_SPAWN_FRAME 7U
#define BATTLE_PROJECTILE_GETSUGA 0U
#define BATTLE_PROJECTILE_VIZARD 1U
#define BATTLE_PROJECTILE_NINETAILS_BOMB 2U
#define BATTLE_VIZARD_TELEPORT_FRAME 3U
#define BATTLE_VIZARD_PROJECTILE_FRAME 5U
#define BATTLE_VIZARD_TELEPORT_GAP 42
#define BATTLE_NINETAILS_BOMB_SPEED 18
#define BATTLE_NINETAILS_BOMB_FRAME_MS 80U
#define BATTLE_NINETAILS_BOMB_SPAWN_FRAME 0U
#define BATTLE_AI_REACTION_MS 220U
#define BATTLE_AI_ATTACK_RANGE 54
#define BATTLE_AI_SKILL_RANGE 92
#define BATTLE_AI_NEAR_RANGE 72
#define BATTLE_AI_FAR_RANGE 132
#define BATTLE_AI_ACTION_MIN_MS 120U
#define BATTLE_AI_ACTION_MAX_MS 520U
#define BATTLE_OVER_INPUT_DELAY_MS 500U
#define BATTLE_OVER_CONFIRM_MASK \
  (COMBAT_INPUT_ATTACK | COMBAT_INPUT_SKILL | COMBAT_INPUT_JUMP | COMBAT_INPUT_DASH)

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
#define RGB565_HUD_BG 0x0842U
#define RGB565_HUD_FRAME 0xC618U
#define RGB565_HP_BACK 0x4208U
#define RGB565_HP_GOOD 0x07E0U
#define RGB565_HP_WARN 0xFD20U
#define RGB565_MANA 0x04FFU
#define RGB565_MANA_BACK 0x18C6U
#define RGB565_HURTBOX 0x07E0U
#define RGB565_HITBOX 0xF800U

typedef struct
{
  const uint16_t *pixels;
  const void *sourcePixels;
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
  uint8_t kind;
  uint8_t hitConnected;
  uint32_t startedMs;
} BattleProjectile;

typedef enum
{
  BATTLE_AI_ACTION_HOLD = 0U,
  BATTLE_AI_ACTION_APPROACH,
  BATTLE_AI_ACTION_RETREAT,
  BATTLE_AI_ACTION_BLOCK,
  BATTLE_AI_ACTION_ATTACK,
  BATTLE_AI_ACTION_SKILL,
  BATTLE_AI_ACTION_DASH,
  BATTLE_AI_ACTION_JUMP
} BattleAiAction;

typedef struct
{
  int16_t selfX;
  int16_t targetX;
  int16_t targetY;
  uint16_t selfHp;
  uint16_t targetHp;
  CombatAnimState selfState;
  CombatAnimState targetState;
  uint8_t targetFrameIndex;
  uint8_t targetAttackStep;
  uint8_t targetOnGround;
  uint8_t valid;
} BattleAiObservation;

typedef struct
{
  BattleAiObservation visible;
  BattleAiObservation pending;
  BattleAiAction action;
  BattleAiAction queuedAction;
  uint32_t pendingSeenMs;
  uint32_t actionStartedMs;
  uint32_t actionDurationMs;
  uint32_t nextThinkMs;
  uint32_t rng;
  uint8_t difficulty;
} BattleAiController;

static CombatActor s_player;
static CombatActor s_cpu;
static BattleProjectile s_getsuga;
static BattleProjectile s_ninetailsBomb;
static BattleAiController s_cpuAi;
static BattleActorSnapshot s_playerSnapshot;
static BattleActorSnapshot s_cpuSnapshot;
static BattleActorSnapshot s_getsugaSnapshot;
static BattleActorSnapshot s_chidoriSnapshot;
static BattleActorSnapshot s_ninetailsBombSnapshot;
static uint16_t s_playerDecompressBuf[185U * 130U];
static uint16_t s_cpuDecompressBuf[185U * 130U];
static uint16_t s_projDecompressBuf[115U * 70U];

static void Battle_DecompressRLE(uint16_t *dest, const uint16_t *src, uint32_t totalPixels);

static uint16_t s_compositeRun[LCD_PORT_WIDTH];
static uint32_t s_lastTickMs;
static uint16_t s_lastPlayerHp;
static uint16_t s_lastCpuHp;
static uint16_t s_lastPlayerMana;
static uint16_t s_lastCpuMana;
static uint32_t s_lastGetsugaSkillStartMs;
static uint32_t s_lastVizardSkillStartMs;
static uint32_t s_lastVizardProjectileSkillStartMs;
static uint32_t s_lastNinetailsBombSkillStartMs;
static uint32_t s_battleOverStartedMs;
static uint8_t s_vizardTeleported;
static uint8_t s_battleOver;
static uint8_t s_battleWon;
static uint8_t s_battleOverInputArmed;
static uint8_t s_secondsLeft;
static uint32_t s_lastSecondUpdateMs;
static CombatCharacterId s_playerCharacter = COMBAT_CHARACTER_VIZARD_ICHIGO;
static CombatCharacterId s_cpuCharacter = COMBAT_CHARACTER_SASUKE;
static uint8_t s_vsPlayer = 0U;

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

static const CombatHitboxDef s_vizardProjectileHitbox = {
    0U,
    0U,
    {6, 24, 46, 38},
    16U,
    3U,
    320U,
    170U,
    24,
    0,
    COMBAT_HIT_LEVEL_PROJECTILE,
    1U,
};

static const CombatHitboxDef s_ninetailsBombHitbox = {
    0U,
    0U,
    {10, 14, 56, 42},
    20U,
    4U,
    350U,
    180U,
    28,
    0,
    COMBAT_HIT_LEVEL_PROJECTILE,
    1U,
};

static void Battle_DrawFrame(void);
static void Battle_DrawActorsInitial(void);
static void Battle_UpdateActors(void);
static void Battle_AiInit(BattleAiController *ai, uint8_t difficulty, uint32_t nowMs);
static uint8_t Battle_AiUpdate(BattleAiController *ai,
                               const CombatActor *self,
                               const CombatActor *target,
                               uint32_t nowMs);
static void Battle_AiObserve(BattleAiController *ai,
                             const CombatActor *self,
                             const CombatActor *target,
                             uint32_t nowMs);
static BattleAiAction Battle_AiChooseAction(BattleAiController *ai,
                                            const CombatActor *self,
                                            const CombatActor *target,
                                            uint32_t nowMs);
static uint8_t Battle_AiActionToInput(BattleAiAction action,
                                      const CombatActor *self,
                                      const CombatActor *target,
                                      uint32_t nowMs);
static uint16_t Battle_AiRandom(BattleAiController *ai, uint16_t maxValue);
static int16_t Battle_AbsI16(int16_t value);
static void Battle_UpdateVizardSkill(uint32_t nowMs);
static void Battle_TrySpawnGetsuga(uint32_t nowMs);
static void Battle_UpdateGetsuga(uint32_t nowMs);
static void Battle_TrySpawnNinetailsBomb(uint32_t nowMs);
static void Battle_UpdateNinetailsBomb(uint32_t nowMs);
static uint8_t Battle_CaptureNinetailsBomb(const BattleProjectile *projectile,
                                           BattleActorSnapshot *snapshot);
static void Battle_ResolveProjectileHit(BattleProjectile *projectile,
                                        CombatActor *owner,
                                        CombatActor *target,
                                        uint32_t nowMs);
static void Battle_DrawBackground(void);
static void Battle_DrawStaticSky(void);
static void Battle_DrawGround(void);
static void Battle_DrawArenaMarks(void);
static void Battle_DrawHud(void);
static void Battle_DrawBattleOverHud(uint8_t won);
static uint8_t Battle_UpdateBattleOver(uint32_t nowMs);
static void Battle_CheckBattleOver(uint32_t nowMs);
static void Battle_DrawHealthBar(uint16_t x, uint16_t y, uint16_t hp,
                                 uint16_t borderColor, uint8_t reverse);
static void Battle_UpdateHealthBar(uint16_t x, uint16_t y, uint16_t oldHp,
                                   uint16_t newHp, uint16_t borderColor,
                                   uint8_t reverse);
static void Battle_DrawManaBar(uint16_t x, uint16_t y, uint16_t mana,
                               uint8_t reverse);
static void Battle_UpdateManaBar(uint16_t x, uint16_t y, uint16_t oldMana,
                                 uint16_t newMana, uint8_t reverse);
static void Battle_DrawMeter(uint16_t x, uint16_t y, uint16_t width,
                             uint16_t height, uint16_t value,
                             uint16_t maxValue, uint16_t fillColor,
                             uint16_t backColor, uint16_t borderColor,
                             uint8_t reverse);
static void Battle_UpdateMeter(uint16_t x, uint16_t y, uint16_t width,
                               uint16_t height, uint16_t oldValue,
                               uint16_t newValue, uint16_t maxValue,
                               uint16_t fillColor, uint16_t backColor,
                               uint8_t reverse);
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
                                      const BattleActorSnapshot *chidori,
                                      const BattleActorSnapshot *ninetailsBomb,
                                      uint8_t allowSplit);
static void Battle_DrawDirtyRect(BattleDirtyRect rect,
                                 const BattleActorSnapshot *player,
                                 const BattleActorSnapshot *cpu,
                                 const BattleActorSnapshot *projectile,
                                 const BattleActorSnapshot *chidori,
                                 const BattleActorSnapshot *ninetailsBomb);
static uint8_t Battle_DirtyRectsShouldSplit(BattleDirtyRect a,
                                            BattleDirtyRect b);
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
static void Battle_FillSafe(int16_t x, int16_t y, int16_t width,
                            int16_t height, uint16_t color);

void BattleDemo_SetCharacters(CombatCharacterId playerCharacter,
                              CombatCharacterId cpuCharacter)
{
  s_playerCharacter = playerCharacter;
  s_cpuCharacter = cpuCharacter;
}

void BattleDemo_SetVsPlayer(uint8_t enabled)
{
  s_vsPlayer = (enabled != 0U) ? 1U : 0U;
}

void BattleDemo_Init(uint8_t difficulty)
{
  uint32_t now = HAL_GetTick();

  LCD_Port_Init();
  CombatInput_Init();
  CombatActor_Init(&s_player,
                   s_playerCharacter,
                   BATTLE_P1_START_X,
                   BATTLE_GROUND_Y,
                   1,
                   now);
  CombatActor_Init(&s_cpu,
                   s_cpuCharacter,
                   BATTLE_CPU_START_X,
                   BATTLE_GROUND_Y,
                   -1,
                   now);
  s_lastTickMs = now;
  s_lastPlayerHp = s_player.hp;
  s_lastCpuHp = s_cpu.hp;
  s_lastPlayerMana = s_player.mana;
  s_lastCpuMana = s_cpu.mana;
  s_playerSnapshot.valid = 0U;
  s_cpuSnapshot.valid = 0U;
  s_getsugaSnapshot.valid = 0U;
  s_chidoriSnapshot.valid = 0U;
  s_ninetailsBombSnapshot.valid = 0U;
  s_getsuga.active = 0U;
  s_getsuga.kind = BATTLE_PROJECTILE_GETSUGA;
  s_ninetailsBomb.active = 0U;
  s_lastGetsugaSkillStartMs = 0U;
  s_lastVizardSkillStartMs = 0U;
  s_lastVizardProjectileSkillStartMs = 0U;
  s_lastNinetailsBombSkillStartMs = 0U;
  s_battleOverStartedMs = 0U;
  s_vizardTeleported = 0U;
  s_battleOver = 0U;
  s_battleWon = 0U;
  s_battleOverInputArmed = 0U;
  s_secondsLeft = 60U;
  s_lastSecondUpdateMs = now;
  Battle_AiInit(&s_cpuAi, difficulty, now);

  Battle_DrawFrame();
  LCD_Port_Flush();
}

uint8_t BattleDemo_Update(void)
{
  uint32_t now = HAL_GetTick();

  if ((now - s_lastTickMs) < BATTLE_TICK_MS)
  {
    return 0U;
  }

  s_lastTickMs += BATTLE_TICK_MS;

  /* Update countdown timer once per 1000ms */
  if ((now - s_lastSecondUpdateMs) >= 1000U)
  {
    s_lastSecondUpdateMs += 1000U;
    if (s_secondsLeft > 0U)
    {
      s_secondsLeft--;

      char timerStr[3];
      timerStr[0] = (char)('0' + (s_secondsLeft / 10U));
      timerStr[1] = (char)('0' + (s_secondsLeft % 10U));
      timerStr[2] = '\0';

      LCD_Port_FillRect(143U, 6U, 34U, 21U, RGB565_BLACK);
      ILI9341_DrawText(timerStr, FONT3, 148U, 8U, RGB565_WHITE, RGB565_BLACK);
    }
  }

  if (s_battleOver != 0U)
  {
    return Battle_UpdateBattleOver(now);
  }

  uint8_t input = CombatInput_Read();
  uint8_t cpuInput = (s_vsPlayer != 0U)
                         ? CombatInput_ReadPlayer2()
                         : Battle_AiUpdate(&s_cpuAi, &s_cpu, &s_player, now);

  CombatActor_FaceToward(&s_player, &s_cpu);
  CombatActor_FaceToward(&s_cpu, &s_player);
  CombatActor_Update(&s_player, input, now, 1U);
  CombatActor_Update(&s_cpu, cpuInput, now, 1U);
  Battle_UpdateVizardSkill(now);
  Battle_ResolveHit(&s_player, &s_cpu, now);
  Battle_ResolveHit(&s_cpu, &s_player, now);
  Battle_TrySpawnGetsuga(now);
  Battle_UpdateGetsuga(now);
  Battle_ResolveProjectileHit(&s_getsuga, &s_player, &s_cpu, now);
  Battle_TrySpawnNinetailsBomb(now);
  Battle_UpdateNinetailsBomb(now);
  Battle_ResolveProjectileHit(&s_ninetailsBomb, &s_player, &s_cpu, now);

  if (s_player.hp != s_lastPlayerHp)
  {
    Battle_UpdateHealthBar(10U, 15U, s_lastPlayerHp, s_player.hp,
                           RGB565_ACCENT_CYAN, 0U);
    s_lastPlayerHp = s_player.hp;
  }

  if (s_cpu.hp != s_lastCpuHp)
  {
    Battle_UpdateHealthBar(206U, 15U, s_lastCpuHp, s_cpu.hp,
                           RGB565_ACCENT_ORANGE, 1U);
    s_lastCpuHp = s_cpu.hp;
  }

  if (s_player.mana != s_lastPlayerMana)
  {
    Battle_UpdateManaBar(10U, 27U, s_lastPlayerMana, s_player.mana, 0U);
    s_lastPlayerMana = s_player.mana;
  }

  if (s_cpu.mana != s_lastCpuMana)
  {
    Battle_UpdateManaBar(226U, 27U, s_lastCpuMana, s_cpu.mana, 1U);
    s_lastCpuMana = s_cpu.mana;
  }

  Battle_UpdateActors();
  Battle_CheckBattleOver(now);
  LCD_Port_Flush();
  return 0U;
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

  CombatRules_ProcessHit(attacker, target, hitbox, nowMs);

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
  BattleActorSnapshot nextNinetailsBomb;
  uint8_t playerDirty;
  uint8_t cpuDirty;
  uint8_t getsugaDirty;
  uint8_t chidoriDirty;
  uint8_t ninetailsBombDirty;

  Battle_CaptureActor(&s_player, &nextPlayer);
  Battle_CaptureActor(&s_cpu, &nextCpu);
  Battle_CaptureGetsuga(&s_getsuga, &nextGetsuga);
  Battle_CaptureChidori(&s_player, &nextChidori);
  if (nextChidori.valid == 0U)
  {
    Battle_CaptureChidori(&s_cpu, &nextChidori);
  }
  Battle_CaptureNinetailsBomb(&s_ninetailsBomb, &nextNinetailsBomb);

  playerDirty = (Battle_ActorSnapshotEqual(&s_playerSnapshot, &nextPlayer) == 0U);
  cpuDirty = (Battle_ActorSnapshotEqual(&s_cpuSnapshot, &nextCpu) == 0U);
  getsugaDirty = (Battle_ActorSnapshotEqual(&s_getsugaSnapshot, &nextGetsuga) == 0U);
  chidoriDirty = (Battle_ActorSnapshotEqual(&s_chidoriSnapshot, &nextChidori) == 0U);
  ninetailsBombDirty = (Battle_ActorSnapshotEqual(&s_ninetailsBombSnapshot, &nextNinetailsBomb) == 0U);

  if ((playerDirty == 0U) && (cpuDirty == 0U) &&
      (getsugaDirty == 0U) && (chidoriDirty == 0U) &&
      (ninetailsBombDirty == 0U))
  {
    return;
  }

  if (playerDirty != 0U)
  {
    uint8_t allowPlayerSplit =
        ((s_player.character == COMBAT_CHARACTER_VIZARD_ICHIGO) &&
         (s_player.state == COMBAT_ANIM_SKILL) &&
         (s_vizardTeleported != 0U))
            ? 1U
            : 0U;

    Battle_DrawSnapshotChange(&s_playerSnapshot,
                              &nextPlayer,
                              &nextPlayer,
                              &nextCpu,
                              &nextGetsuga,
                              &nextChidori,
                              &nextNinetailsBomb,
                              allowPlayerSplit);
  }

  if (cpuDirty != 0U)
  {
    Battle_DrawSnapshotChange(&s_cpuSnapshot,
                              &nextCpu,
                              &nextPlayer,
                              &nextCpu,
                              &nextGetsuga,
                              &nextChidori,
                              &nextNinetailsBomb,
                              0U);
  }

  if (getsugaDirty != 0U)
  {
    Battle_DrawSnapshotChange(&s_getsugaSnapshot,
                              &nextGetsuga,
                              &nextPlayer,
                              &nextCpu,
                              &nextGetsuga,
                              &nextChidori,
                              &nextNinetailsBomb,
                              0U);
  }

  if (chidoriDirty != 0U)
  {
    Battle_DrawSnapshotChange(&s_chidoriSnapshot,
                              &nextChidori,
                              &nextPlayer,
                              &nextCpu,
                              &nextGetsuga,
                              &nextChidori,
                              &nextNinetailsBomb,
                              0U);
  }

  if (ninetailsBombDirty != 0U)
  {
    Battle_DrawSnapshotChange(&s_ninetailsBombSnapshot,
                              &nextNinetailsBomb,
                              &nextPlayer,
                              &nextCpu,
                              &nextGetsuga,
                              &nextChidori,
                              &nextNinetailsBomb,
                              0U);
  }

  s_playerSnapshot = nextPlayer;
  s_cpuSnapshot = nextCpu;
  s_getsugaSnapshot = nextGetsuga;
  s_chidoriSnapshot = nextChidori;
  s_ninetailsBombSnapshot = nextNinetailsBomb;
}

static void Battle_AiInit(BattleAiController *ai, uint8_t difficulty, uint32_t nowMs)
{
  if (ai == 0)
  {
    return;
  }

  ai->visible.valid = 0U;
  ai->pending.valid = 0U;
  ai->action = BATTLE_AI_ACTION_HOLD;
  ai->queuedAction = BATTLE_AI_ACTION_HOLD;
  ai->pendingSeenMs = nowMs;
  ai->actionStartedMs = nowMs;
  ai->actionDurationMs = BATTLE_AI_ACTION_MIN_MS;
  ai->nextThinkMs = nowMs;
  ai->rng = 0x13572468UL;
  ai->difficulty = difficulty;
}

static uint8_t Battle_AiUpdate(BattleAiController *ai,
                               const CombatActor *self,
                               const CombatActor *target,
                               uint32_t nowMs)
{
  if ((ai == 0) || (self == 0) || (target == 0))
  {
    return COMBAT_INPUT_NONE;
  }

  Battle_AiObserve(ai, self, target, nowMs);

  if ((self->state == COMBAT_ANIM_HIT) || (self->state == COMBAT_ANIM_DEAD))
  {
    uint32_t reactionMs = 220U;
    if (ai->difficulty == 0U)
    {
      reactionMs = 450U;
    }
    else if (ai->difficulty == 2U)
    {
      reactionMs = 110U;
    }

    ai->action = BATTLE_AI_ACTION_HOLD;
    ai->queuedAction = BATTLE_AI_ACTION_HOLD;
    ai->actionStartedMs = nowMs;
    ai->nextThinkMs = nowMs + reactionMs;
    return COMBAT_INPUT_NONE;
  }

  uint32_t elapsed = nowMs - ai->actionStartedMs;
  uint8_t locked = CombatActor_IsActionLocked(self, nowMs);
  uint32_t thinkPoint =
      (ai->actionDurationMs * 65U) / 100U;

  if ((self->state == COMBAT_ANIM_ATTACK) &&
      (self->attackStep < 2U))
  {
    return COMBAT_INPUT_ATTACK;
  }

  if ((elapsed >= thinkPoint) && (nowMs >= ai->nextThinkMs))
  {
    ai->queuedAction = Battle_AiChooseAction(ai, self, target, nowMs);
    ai->nextThinkMs = nowMs + 110U + Battle_AiRandom(ai, 90U);
  }

  if ((locked == 0U) &&
      ((elapsed >= ai->actionDurationMs) ||
       (ai->action == BATTLE_AI_ACTION_HOLD)))
  {
    if (ai->action == BATTLE_AI_ACTION_HOLD)
    {
      ai->queuedAction = Battle_AiChooseAction(ai, self, target, nowMs);
    }
    ai->action = ai->queuedAction;
    ai->queuedAction = BATTLE_AI_ACTION_HOLD;
    ai->actionStartedMs = nowMs;
    ai->actionDurationMs = BATTLE_AI_ACTION_MIN_MS +
                           Battle_AiRandom(ai,
                                           (uint16_t)(BATTLE_AI_ACTION_MAX_MS -
                                                      BATTLE_AI_ACTION_MIN_MS));
  }

  return Battle_AiActionToInput(ai->action, self, target, nowMs);
}

static void Battle_AiObserve(BattleAiController *ai,
                             const CombatActor *self,
                             const CombatActor *target,
                             uint32_t nowMs)
{
  if ((ai == 0) || (self == 0) || (target == 0))
  {
    return;
  }

  ai->pending.selfX = self->x;
  ai->pending.targetX = target->x;
  ai->pending.targetY = target->y;
  ai->pending.selfHp = self->hp;
  ai->pending.targetHp = target->hp;
  ai->pending.selfState = self->state;
  ai->pending.targetState = target->state;
  ai->pending.targetFrameIndex = target->frameIndex;
  ai->pending.targetAttackStep = target->attackStep;
  ai->pending.targetOnGround = target->onGround;
  ai->pending.valid = 1U;

  uint32_t reactionMs = 220U;
  if (ai->difficulty == 0U)
  {
    reactionMs = 450U;
  }
  else if (ai->difficulty == 2U)
  {
    reactionMs = 110U;
  }

  if ((ai->visible.valid == 0U) ||
      ((nowMs - ai->pendingSeenMs) >= reactionMs))
  {
    ai->visible = ai->pending;
    ai->pendingSeenMs = nowMs;
  }
}

static BattleAiAction Battle_AiChooseAction(BattleAiController *ai,
                                            const CombatActor *self,
                                            const CombatActor *target,
                                            uint32_t nowMs)
{
  (void)nowMs;

  if ((ai == 0) || (self == 0) || (target == 0) ||
      (ai->visible.valid == 0U))
  {
    return BATTLE_AI_ACTION_HOLD;
  }

  int16_t dx = (int16_t)(ai->visible.targetX - ai->visible.selfX);
  int16_t distance = Battle_AbsI16(dx);
  uint8_t targetAttacking =
      (ai->visible.targetState == COMBAT_ANIM_ATTACK) ||
      (ai->visible.targetState == COMBAT_ANIM_SKILL);
  uint8_t targetActive =
      (targetAttacking != 0U) &&
      (ai->visible.targetFrameIndex >= 2U) &&
      (ai->visible.targetFrameIndex <= 5U);
  uint8_t targetRecovering =
      (targetAttacking != 0U) &&
      (ai->visible.targetFrameIndex > 5U);

  if ((targetRecovering != 0U) &&
      (distance <= BATTLE_AI_ATTACK_RANGE))
  {
    return BATTLE_AI_ACTION_ATTACK;
  }

  if ((ai->visible.targetOnGround == 0U) &&
      (distance <= BATTLE_AI_NEAR_RANGE))
  {
    return BATTLE_AI_ACTION_ATTACK;
  }

  uint16_t blockRate = 60U;
  uint16_t skillRate = 50U;
  uint16_t retreatRate = 40U;

  if (ai->difficulty == 0U)
  {
    blockRate = 25U;
    skillRate = 20U;
    retreatRate = 15U;
  }
  else if (ai->difficulty == 2U)
  {
    blockRate = 90U;
    skillRate = 80U;
    retreatRate = 70U;
  }

  if ((targetActive != 0U) &&
      (distance <= BATTLE_AI_NEAR_RANGE) &&
      (Battle_AiRandom(ai, 100U) < blockRate))
  {
    return BATTLE_AI_ACTION_BLOCK;
  }

  if ((distance <= BATTLE_AI_SKILL_RANGE) &&
      ((ai->visible.targetHp <= 45U) || (ai->visible.selfHp <= 35U)) &&
      (Battle_AiRandom(ai, 100U) < skillRate))
  {
    return BATTLE_AI_ACTION_SKILL;
  }

  if (distance <= BATTLE_AI_ATTACK_RANGE)
  {
    return BATTLE_AI_ACTION_ATTACK;
  }

  if ((distance <= 34) &&
      (ai->visible.selfHp < ai->visible.targetHp) &&
      (Battle_AiRandom(ai, 100U) < retreatRate))
  {
    return BATTLE_AI_ACTION_RETREAT;
  }

  if (distance > BATTLE_AI_FAR_RANGE)
  {
    return BATTLE_AI_ACTION_DASH;
  }

  if (distance > BATTLE_AI_ATTACK_RANGE)
  {
    return BATTLE_AI_ACTION_APPROACH;
  }

  if (Battle_AiRandom(ai, 100U) < 18U)
  {
    return BATTLE_AI_ACTION_JUMP;
  }

  return BATTLE_AI_ACTION_HOLD;
}

static uint8_t Battle_AiActionToInput(BattleAiAction action,
                                      const CombatActor *self,
                                      const CombatActor *target,
                                      uint32_t nowMs)
{
  (void)nowMs;

  if ((self == 0) || (target == 0))
  {
    return COMBAT_INPUT_NONE;
  }

  uint8_t toward = (target->x < self->x) ? COMBAT_INPUT_LEFT
                                         : COMBAT_INPUT_RIGHT;
  uint8_t away = (target->x < self->x) ? COMBAT_INPUT_RIGHT
                                       : COMBAT_INPUT_LEFT;

  switch (action)
  {
    case BATTLE_AI_ACTION_APPROACH:
      return toward;
    case BATTLE_AI_ACTION_RETREAT:
      return away;
    case BATTLE_AI_ACTION_BLOCK:
      return COMBAT_INPUT_BLOCK;
    case BATTLE_AI_ACTION_ATTACK:
      return COMBAT_INPUT_ATTACK;
    case BATTLE_AI_ACTION_SKILL:
      return COMBAT_INPUT_SKILL;
    case BATTLE_AI_ACTION_DASH:
      return (uint8_t)(toward | COMBAT_INPUT_DASH);
    case BATTLE_AI_ACTION_JUMP:
      return COMBAT_INPUT_JUMP;
    default:
      return COMBAT_INPUT_NONE;
  }
}

static uint16_t Battle_AiRandom(BattleAiController *ai, uint16_t maxValue)
{
  if ((ai == 0) || (maxValue == 0U))
  {
    return 0U;
  }

  ai->rng = (ai->rng * 1664525UL) + 1013904223UL;
  return (uint16_t)((ai->rng >> 16) % maxValue);
}

static int16_t Battle_AbsI16(int16_t value)
{
  return (value < 0) ? (int16_t)-value : value;
}

static void Battle_UpdateVizardSkill(uint32_t nowMs)
{
  (void)nowMs;

  if ((s_player.character != COMBAT_CHARACTER_VIZARD_ICHIGO) ||
      (s_player.state != COMBAT_ANIM_SKILL))
  {
    s_vizardTeleported = 0U;
    return;
  }

  if (s_lastVizardSkillStartMs != s_player.stateStartedMs)
  {
    s_lastVizardSkillStartMs = s_player.stateStartedMs;
    s_vizardTeleported = 0U;
  }

  if ((s_vizardTeleported != 0U) ||
      (s_player.frameIndex < BATTLE_VIZARD_TELEPORT_FRAME))
  {
    return;
  }

  int16_t targetX = (s_cpu.facing > 0)
                        ? (int16_t)(s_cpu.x - BATTLE_VIZARD_TELEPORT_GAP)
                        : (int16_t)(s_cpu.x + BATTLE_VIZARD_TELEPORT_GAP);

  if (targetX < 32)
  {
    targetX = 32;
  }
  else if (targetX > 288)
  {
    targetX = 288;
  }

  s_player.x = targetX;
  s_player.vx = 0;
  s_player.vy = 0;
  s_player.facing = (s_cpu.x < s_player.x) ? -1 : 1;
  s_vizardTeleported = 1U;
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

  // Decompress Naruto Nine Tails RLE pixels if needed
  if (actor->character == COMBAT_CHARACTER_NARUTO_FULL_NINE_TAILS)
  {
    uint16_t *decompressBuf = (actor == &s_player) ? s_playerDecompressBuf : s_cpuDecompressBuf;
    uint32_t totalPixels = (uint32_t)frame.width * frame.height;
    snapshot->sourcePixels = frame.pixels;
    if (totalPixels <= (185U * 130U))
    {
      Battle_DecompressRLE(decompressBuf, frame.pixels, totalPixels);
      snapshot->pixels = decompressBuf;
    }
    else
    {
      snapshot->pixels = frame.pixels;
    }
  }
  else
  {
    snapshot->pixels = frame.pixels;
    snapshot->sourcePixels = frame.pixels;
  }

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
  if ((s_player.character == COMBAT_CHARACTER_VIZARD_ICHIGO) &&
      (s_player.state == COMBAT_ANIM_SKILL) &&
      (s_player.frameIndex >= BATTLE_VIZARD_PROJECTILE_FRAME) &&
      (s_vizardTeleported != 0U) &&
      (s_lastVizardProjectileSkillStartMs != s_player.stateStartedMs))
  {
    const VizardMoveAnimation *projectileAnim =
        &vizard_move_animations[VIZARD_MOVE_SKILL_PROJECTILE];
    const VizardMoveFrame *frame = &projectileAnim->frames[0];

    s_lastVizardProjectileSkillStartMs = s_player.stateStartedMs;
    s_getsuga.active = 1U;
    s_getsuga.kind = BATTLE_PROJECTILE_VIZARD;
    s_getsuga.frameIndex = 0U;
    s_getsuga.hitConnected = 0U;
    s_getsuga.startedMs = nowMs;
    s_getsuga.vx = (s_player.facing < 0) ? -BATTLE_GETSUGA_SPEED
                                         : BATTLE_GETSUGA_SPEED;
    s_getsuga.y = (int16_t)(s_player.y - frame->pivotY);

    if (s_player.facing < 0)
    {
      s_getsuga.x = (int16_t)(s_player.x - 18 - (int16_t)frame->width);
    }
    else
    {
      s_getsuga.x = (int16_t)(s_player.x + 18);
    }
    return;
  }

  if ((s_player.character != COMBAT_CHARACTER_ICHIGO) ||
      (s_player.state != COMBAT_ANIM_SKILL) ||
      (s_player.frameIndex < BATTLE_GETSUGA_SPAWN_FRAME) ||
      (s_lastGetsugaSkillStartMs == s_player.stateStartedMs))
  {
    return;
  }

  s_lastGetsugaSkillStartMs = s_player.stateStartedMs;
  s_getsuga.active = 1U;
  s_getsuga.kind = BATTLE_PROJECTILE_GETSUGA;
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

  uint8_t frameCount = GETSUGA_PROJECTILE_FRAME_COUNT;
  uint16_t width = GETSUGA_PROJECTILE_WIDTH;

  if (s_getsuga.kind == BATTLE_PROJECTILE_VIZARD)
  {
    const VizardMoveAnimation *projectileAnim =
        &vizard_move_animations[VIZARD_MOVE_SKILL_PROJECTILE];
    frameCount = projectileAnim->frameCount;
    width = projectileAnim->frames[s_getsuga.frameIndex].width;
  }

  s_getsuga.x = (int16_t)(s_getsuga.x + s_getsuga.vx);
  s_getsuga.frameIndex = (uint8_t)(((nowMs - s_getsuga.startedMs) /
                                    BATTLE_GETSUGA_FRAME_MS) %
                                   frameCount);

  if ((s_getsuga.x > (int16_t)LCD_PORT_WIDTH) ||
      ((s_getsuga.x + (int16_t)width) < 0))
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

  const CombatHitboxDef *hitbox;
  if (projectile->kind == BATTLE_PROJECTILE_VIZARD)
  {
    hitbox = &s_vizardProjectileHitbox;
  }
  else if (projectile->kind == BATTLE_PROJECTILE_NINETAILS_BOMB)
  {
    hitbox = &s_ninetailsBombHitbox;
  }
  else
  {
    hitbox = &s_getsugaHitbox;
  }

  CombatBox projectileBox = {
      (int16_t)(projectile->x + hitbox->box.x),
      (int16_t)(projectile->y + hitbox->box.y),
      hitbox->box.w,
      hitbox->box.h,
  };
  CombatBox hurtboxWorld = CombatActor_GetHurtboxWorld(target);

  if (CombatBox_Overlap(projectileBox, hurtboxWorld) == 0U)
  {
    return;
  }

  CombatRules_ProcessHit(owner, target, hitbox, nowMs);
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

  if (projectile->kind == BATTLE_PROJECTILE_VIZARD)
  {
    const VizardMoveAnimation *projectileAnim =
        &vizard_move_animations[VIZARD_MOVE_SKILL_PROJECTILE];
    const VizardMoveFrame *frame =
        &projectileAnim->frames[projectile->frameIndex];

    snapshot->pixels = frame->pixels;
    snapshot->sourcePixels = frame->pixels;
    snapshot->width = frame->width;
    snapshot->height = frame->height;
  }
  else
  {
    snapshot->pixels = getsuga_projectile_frames[projectile->frameIndex];
    snapshot->sourcePixels = getsuga_projectile_frames[projectile->frameIndex];
    snapshot->width = GETSUGA_PROJECTILE_WIDTH;
    snapshot->height = GETSUGA_PROJECTILE_HEIGHT;
  }

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
  snapshot->sourcePixels = frame->pixels;
  snapshot->width = frame->width;
  snapshot->height = frame->height;
  snapshot->y = (int16_t)(actor->y - 36 - (int16_t)(frame->height / 2U));

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

  if (a->valid != b->valid)
  {
    return 0U;
  }

  if (a->valid == 0U)
  {
    return 1U;
  }

  return ((a->sourcePixels == b->sourcePixels) &&
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
                                      const BattleActorSnapshot *chidori,
                                      const BattleActorSnapshot *ninetailsBomb,
                                      uint8_t allowSplit)
{
  BattleDirtyRect dirty = {0, 0, 0, 0, 0U};
  BattleDirtyRect previousDirty = {0, 0, 0, 0, 0U};
  BattleDirtyRect nextDirty = {0, 0, 0, 0, 0U};

  Battle_DirtyRectAddSnapshot(&previousDirty, previous);
  Battle_DirtyRectAddSnapshot(&nextDirty, next);

  if ((allowSplit != 0U) &&
      (Battle_DirtyRectsShouldSplit(previousDirty, nextDirty) != 0U))
  {
    Battle_DrawDirtyRect(previousDirty, player, cpu, projectile, chidori, ninetailsBomb);
    Battle_DrawDirtyRect(nextDirty, player, cpu, projectile, chidori, ninetailsBomb);
    return;
  }

  Battle_DirtyRectAddSnapshot(&dirty, previous);
  Battle_DirtyRectAddSnapshot(&dirty, next);
  Battle_DrawDirtyRect(dirty, player, cpu, projectile, chidori, ninetailsBomb);
}

static uint8_t Battle_DirtyRectsShouldSplit(BattleDirtyRect a,
                                            BattleDirtyRect b)
{
  if ((a.valid == 0U) || (b.valid == 0U))
  {
    return 0U;
  }

  int16_t ax1 = (int16_t)(a.x + a.w);
  int16_t ay1 = (int16_t)(a.y + a.h);
  int16_t bx1 = (int16_t)(b.x + b.w);
  int16_t by1 = (int16_t)(b.y + b.h);

  int16_t unionX0 = (a.x < b.x) ? a.x : b.x;
  int16_t unionY0 = (a.y < b.y) ? a.y : b.y;
  int16_t unionX1 = (ax1 > bx1) ? ax1 : bx1;
  int16_t unionY1 = (ay1 > by1) ? ay1 : by1;

  uint32_t areaA = (uint32_t)a.w * (uint32_t)a.h;
  uint32_t areaB = (uint32_t)b.w * (uint32_t)b.h;
  uint32_t unionArea = (uint32_t)(unionX1 - unionX0) *
                       (uint32_t)(unionY1 - unionY0);

  return (unionArea > ((areaA + areaB) + ((areaA + areaB) / 2U))) ? 1U : 0U;
}

static void Battle_DrawDirtyRect(BattleDirtyRect rect,
                                 const BattleActorSnapshot *player,
                                 const BattleActorSnapshot *cpu,
                                 const BattleActorSnapshot *projectile,
                                 const BattleActorSnapshot *chidori,
                                 const BattleActorSnapshot *ninetailsBomb)
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
      s_compositeRun[col] = Battle_GetBackgroundPixel(x, y);
    }

    Battle_ComposeActorRow(rect, player, y);
    Battle_ComposeActorRow(rect, cpu, y);
    Battle_ComposeActorRow(rect, projectile, y);
    Battle_ComposeActorRow(rect, chidori, y);
    Battle_ComposeActorRow(rect, ninetailsBomb, y);

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
  LCD_Port_FillRect(0U, 0U, LCD_PORT_WIDTH, 37U, RGB565_HUD_BG);
  LCD_Port_FillRect(0U, 36U, LCD_PORT_WIDTH, 1U, RGB565_HUD_FRAME);

  ILI9341_DrawText("P1", FONT1, 10U, 3U, RGB565_WHITE, RGB565_HUD_BG);
  ILI9341_DrawText((s_vsPlayer != 0U) ? "P2" : "CPU",
                   FONT1,
                   (s_vsPlayer != 0U) ? 286U : 274U,
                   3U,
                   RGB565_WHITE,
                   RGB565_HUD_BG);

  Battle_DrawHealthBar(10U, 15U, s_player.hp, RGB565_ACCENT_CYAN, 0U);
  Battle_DrawManaBar(10U, 27U, s_player.mana, 0U);

  Battle_DrawHealthBar(206U, 15U, s_cpu.hp, RGB565_ACCENT_ORANGE, 1U);
  Battle_DrawManaBar(226U, 27U, s_cpu.mana, 1U);

  ILI9341_DrawHollowRectangleCoord(142U, 5U, 177U, 27U, RGB565_HUD_FRAME);
  LCD_Port_FillRect(143U, 6U, 34U, 21U, RGB565_BLACK);
  ILI9341_DrawText("60", FONT3, 148U, 8U, RGB565_WHITE, RGB565_BLACK);
}

static void Battle_DrawBattleOverHud(uint8_t won)
{
  uint16_t accent = (won != 0U) ? RGB565_ACCENT_CYAN : RGB565_ACCENT_ORANGE;

  LCD_Port_FillRect(40U, 70U, 240U, 98U, RGB565_HUD_BG);
  ILI9341_DrawHollowRectangleCoord(40U, 70U, 279U, 167U, accent);
  ILI9341_DrawHollowRectangleCoord(43U, 73U, 276U, 164U, RGB565_HUD_FRAME);

  if (s_vsPlayer != 0U)
  {
    ILI9341_DrawText((won != 0U) ? "P1 WINS" : "P2 WINS",
                     FONT4,
                     104U,
                     91U,
                     RGB565_WHITE,
                     RGB565_HUD_BG);
    ILI9341_DrawText("PLAYER BATTLE", FONT2, 98U, 119U, accent, RGB565_HUD_BG);
  }
  else if (won != 0U)
  {
    ILI9341_DrawText("YOU WIN", FONT4, 111U, 91U, RGB565_WHITE, RGB565_HUD_BG);
    ILI9341_DrawText("CPU DEFEATED", FONT2, 101U, 119U, accent, RGB565_HUD_BG);
  }
  else
  {
    ILI9341_DrawText("GAME OVER", FONT4, 96U, 91U, RGB565_WHITE, RGB565_HUD_BG);
    ILI9341_DrawText("PLAYER DOWN", FONT2, 104U, 119U, accent, RGB565_HUD_BG);
  }

  ILI9341_DrawText("ATTACK: MAIN MENU", FONT1, 91U, 146U,
                   RGB565_WHITE, RGB565_HUD_BG);
}

static uint8_t Battle_UpdateBattleOver(uint32_t nowMs)
{
  uint8_t input = CombatInput_Read();
  uint8_t p2Input = (s_vsPlayer != 0U) ? CombatInput_ReadPlayer2() : 0U;

  if ((uint32_t)(nowMs - s_battleOverStartedMs) < BATTLE_OVER_INPUT_DELAY_MS)
  {
    return 0U;
  }

  if (((input | p2Input) & BATTLE_OVER_CONFIRM_MASK) == 0U)
  {
    s_battleOverInputArmed = 1U;
    return 0U;
  }

  return (s_battleOverInputArmed != 0U) ? 1U : 0U;
}

static void Battle_CheckBattleOver(uint32_t nowMs)
{
  if (s_battleOver != 0U)
  {
    return;
  }

  if ((s_player.hp != 0U) && (s_cpu.hp != 0U) && (s_secondsLeft != 0U))
  {
    return;
  }

  s_battleOver = 1U;
  if (s_secondsLeft == 0U)
  {
    s_battleWon = (s_player.hp >= s_cpu.hp) ? 1U : 0U;
  }
  else
  {
    s_battleWon = (s_cpu.hp == 0U) && (s_player.hp != 0U) ? 1U : 0U;
  }
  s_battleOverStartedMs = nowMs;
  s_battleOverInputArmed = 0U;
  s_getsuga.active = 0U;
  Battle_DrawBattleOverHud(s_battleWon);

  if (s_battleWon != 0U)
  {
    Buzzer_Play(BUZZER_SFX_GAME_WIN);
  }
  else
  {
    Buzzer_Play(BUZZER_SFX_GAME_LOSE);
  }
}

static void Battle_DrawHealthBar(uint16_t x, uint16_t y, uint16_t hp,
                                 uint16_t borderColor, uint8_t reverse)
{
  uint16_t fill = (hp > 30U) ? RGB565_HP_GOOD : RGB565_HP_WARN;

  Battle_DrawMeter(x, y, 104U, 8U, hp, 100U, fill, RGB565_HP_BACK,
                   borderColor, reverse);
}

static void Battle_UpdateHealthBar(uint16_t x, uint16_t y, uint16_t oldHp,
                                   uint16_t newHp, uint16_t borderColor,
                                   uint8_t reverse)
{
  uint16_t oldFill = (oldHp > 30U) ? RGB565_HP_GOOD : RGB565_HP_WARN;
  uint16_t newFill = (newHp > 30U) ? RGB565_HP_GOOD : RGB565_HP_WARN;
  uint16_t innerWidth = 102U;
  uint16_t innerHeight = 6U;
  uint16_t oldValue = (oldHp > 100U) ? 100U : oldHp;
  uint16_t newValue = (newHp > 100U) ? 100U : newHp;
  uint16_t oldFillWidth =
      (uint16_t)(((uint32_t)innerWidth * oldValue) / 100U);
  uint16_t newFillWidth =
      (uint16_t)(((uint32_t)innerWidth * newValue) / 100U);
  uint16_t changedWidth;
  uint16_t changedX;

  if (oldFill != newFill)
  {
    Battle_DrawHealthBar(x, y, newHp, borderColor, reverse);
    return;
  }

  if (oldFillWidth == newFillWidth)
  {
    return;
  }

  changedWidth = (oldFillWidth > newFillWidth)
                     ? (uint16_t)(oldFillWidth - newFillWidth)
                     : (uint16_t)(newFillWidth - oldFillWidth);

  if (newFillWidth > oldFillWidth)
  {
    changedX = (reverse != 0U)
                   ? (uint16_t)(x + 1U + innerWidth - newFillWidth)
                   : (uint16_t)(x + 1U + oldFillWidth);
    LCD_Port_FillRect(changedX, (uint16_t)(y + 1U), changedWidth,
                      innerHeight, newFill);
    return;
  }

  changedX = (reverse != 0U)
                 ? (uint16_t)(x + 1U + innerWidth - oldFillWidth)
                 : (uint16_t)(x + 1U + newFillWidth);
  LCD_Port_FillRect(changedX, (uint16_t)(y + 1U), changedWidth,
                    innerHeight, RGB565_HP_BACK);
}

static void Battle_DrawManaBar(uint16_t x, uint16_t y, uint16_t mana, uint8_t reverse)
{
  Battle_DrawMeter(x, y, 84U, 5U, mana, 100U, RGB565_MANA,
                   RGB565_MANA_BACK, RGB565_HUD_FRAME, reverse);
}

static void Battle_UpdateManaBar(uint16_t x, uint16_t y, uint16_t oldMana,
                                 uint16_t newMana, uint8_t reverse)
{
  Battle_UpdateMeter(x, y, 84U, 5U, oldMana, newMana, 100U, RGB565_MANA,
                     RGB565_MANA_BACK, reverse);
}

static void Battle_DrawMeter(uint16_t x, uint16_t y, uint16_t width,
                             uint16_t height, uint16_t value,
                             uint16_t maxValue, uint16_t fillColor,
                             uint16_t backColor, uint16_t borderColor,
                             uint8_t reverse)
{
  if ((width < 3U) || (height < 3U) || (maxValue == 0U))
  {
    return;
  }

  uint16_t innerWidth = (uint16_t)(width - 2U);
  uint16_t innerHeight = (uint16_t)(height - 2U);
  uint16_t clampedValue = (value > maxValue) ? maxValue : value;
  uint16_t fillWidth =
      (uint16_t)(((uint32_t)innerWidth * clampedValue) / maxValue);

  ILI9341_DrawHollowRectangleCoord(x, y,
                                   (uint16_t)(x + width - 1U),
                                   (uint16_t)(y + height - 1U),
                                   borderColor);
  LCD_Port_FillRect((uint16_t)(x + 1U), (uint16_t)(y + 1U),
                    innerWidth, innerHeight, backColor);

  if (fillWidth == 0U)
  {
    return;
  }

  uint16_t fillX = (reverse != 0U)
                       ? (uint16_t)(x + 1U + innerWidth - fillWidth)
                       : (uint16_t)(x + 1U);
  LCD_Port_FillRect(fillX, (uint16_t)(y + 1U), fillWidth, innerHeight,
                    fillColor);
}

static void Battle_UpdateMeter(uint16_t x, uint16_t y, uint16_t width,
                               uint16_t height, uint16_t oldValue,
                               uint16_t newValue, uint16_t maxValue,
                               uint16_t fillColor, uint16_t backColor,
                               uint8_t reverse)
{
  if ((width < 3U) || (height < 3U) || (maxValue == 0U))
  {
    return;
  }

  uint16_t innerWidth = (uint16_t)(width - 2U);
  uint16_t innerHeight = (uint16_t)(height - 2U);
  uint16_t oldClamped = (oldValue > maxValue) ? maxValue : oldValue;
  uint16_t newClamped = (newValue > maxValue) ? maxValue : newValue;
  uint16_t oldFillWidth =
      (uint16_t)(((uint32_t)innerWidth * oldClamped) / maxValue);
  uint16_t newFillWidth =
      (uint16_t)(((uint32_t)innerWidth * newClamped) / maxValue);
  uint16_t changedWidth;
  uint16_t changedX;

  if (oldFillWidth == newFillWidth)
  {
    return;
  }

  changedWidth = (oldFillWidth > newFillWidth)
                     ? (uint16_t)(oldFillWidth - newFillWidth)
                     : (uint16_t)(newFillWidth - oldFillWidth);

  if (newFillWidth > oldFillWidth)
  {
    changedX = (reverse != 0U)
                   ? (uint16_t)(x + 1U + innerWidth - newFillWidth)
                   : (uint16_t)(x + 1U + oldFillWidth);
    LCD_Port_FillRect(changedX, (uint16_t)(y + 1U), changedWidth,
                      innerHeight, fillColor);
    return;
  }

  changedX = (reverse != 0U)
                 ? (uint16_t)(x + 1U + innerWidth - oldFillWidth)
                 : (uint16_t)(x + 1U + newFillWidth);
  LCD_Port_FillRect(changedX, (uint16_t)(y + 1U), changedWidth,
                    innerHeight, backColor);
}



static void Battle_DrawBackground(void)
{
  Battle_DrawStaticSky();
  Battle_DrawGround();
  Battle_DrawArenaMarks();
}

static void Battle_DrawStaticSky(void)
{
  LCD_Port_FillRect(0U, 0U, LCD_PORT_WIDTH, 48U, RGB565_SKY_TOP);
  LCD_Port_FillRect(0U, 48U, LCD_PORT_WIDTH, 48U, RGB565_SKY_MID);
  LCD_Port_FillRect(0U, 96U, LCD_PORT_WIDTH, 86U, RGB565_SKY_LOW);
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
  else if (y >= 182U)
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

/* --------------------------------------------------------------------------
 * Naruto Full Nine Tails -- SKILL1 quả bom bay đi (projectile)
 * -------------------------------------------------------------------------- */

static void Battle_TrySpawnNinetailsBomb(uint32_t nowMs)
{
  /* Chỉ kích hoạt khi player là Naruto chín đuôi, đang dùng skill, tại frame 0,
   * và chưa sinh bom cho lần skill này */
  if ((s_player.character != COMBAT_CHARACTER_NARUTO_FULL_NINE_TAILS) ||
      (s_player.state != COMBAT_ANIM_SKILL) ||
      (s_player.frameIndex < BATTLE_NINETAILS_BOMB_SPAWN_FRAME) ||
      (s_lastNinetailsBombSkillStartMs == s_player.stateStartedMs))
  {
    return;
  }

  const NarutoFullNineTailsMoveAnimation *anim =
      &naruto_full_nine_tails_move_animations[NARUTO_FULL_NINE_TAILS_MOVE_SKILL1];

  if ((anim->projectileFrames == 0) || (anim->projectileFrameCount == 0U))
  {
    return;
  }

  s_lastNinetailsBombSkillStartMs = s_player.stateStartedMs;
  s_ninetailsBomb.active       = 1U;
  s_ninetailsBomb.kind         = BATTLE_PROJECTILE_NINETAILS_BOMB;
  s_ninetailsBomb.frameIndex   = 0U;
  s_ninetailsBomb.hitConnected = 0U;
  s_ninetailsBomb.startedMs    = nowMs;
  s_ninetailsBomb.vx = (s_player.facing < 0) ? -BATTLE_NINETAILS_BOMB_SPEED
                                              :  BATTLE_NINETAILS_BOMB_SPEED;

  /* Y: căn đáy bom sát mặt đất (bom cao 67px, mặt đất ở BATTLE_GROUND_Y) */
  s_ninetailsBomb.y = (int16_t)(BATTLE_GROUND_Y - (int16_t)anim->projectileHeight);

  /* X: sinh sát mép nhân vật theo hướng quay mặt */
  if (s_player.facing < 0)
  {
    /* Quay trái: bom xuất hiện bên trái nhân vật */
    s_ninetailsBomb.x = (int16_t)(s_player.x - 18 - (int16_t)anim->projectileWidth);
  }
  else
  {
    /* Quay phải: bom xuất hiện bên phải nhân vật */
    s_ninetailsBomb.x = (int16_t)(s_player.x + 18);
  }
}

static void Battle_UpdateNinetailsBomb(uint32_t nowMs)
{
  if (s_ninetailsBomb.active == 0U)
  {
    return;
  }

  const NarutoFullNineTailsMoveAnimation *anim =
      &naruto_full_nine_tails_move_animations[NARUTO_FULL_NINE_TAILS_MOVE_SKILL1];

  uint8_t  frameCount = anim->projectileFrameCount;
  uint16_t width      = anim->projectileWidth;

  /* Di chuyển theo trục X */
  s_ninetailsBomb.x = (int16_t)(s_ninetailsBomb.x + s_ninetailsBomb.vx);

  /* Cập nhật frame hoạt ảnh (looping) */
  s_ninetailsBomb.frameIndex =
      (uint8_t)(((nowMs - s_ninetailsBomb.startedMs) / BATTLE_NINETAILS_BOMB_FRAME_MS) %
                frameCount);

  /* Hủy bom khi bay ra khỏi màn hình */
  if ((s_ninetailsBomb.x > (int16_t)LCD_PORT_WIDTH) ||
      ((s_ninetailsBomb.x + (int16_t)width) < 0))
  {
    s_ninetailsBomb.active = 0U;
  }
}

static uint8_t Battle_CaptureNinetailsBomb(const BattleProjectile *projectile,
                                           BattleActorSnapshot *snapshot)
{
  if (snapshot != 0)
  {
    snapshot->valid = 0U;
  }

  if ((projectile == 0) || (snapshot == 0) || (projectile->active == 0U))
  {
    return 0U;
  }

  const NarutoFullNineTailsMoveAnimation *anim =
      &naruto_full_nine_tails_move_animations[NARUTO_FULL_NINE_TAILS_MOVE_SKILL1];

  if ((anim->projectileFrames == 0) || (anim->projectileFrameCount == 0U))
  {
    return 0U;
  }

  uint8_t idx = projectile->frameIndex;
  if (idx >= anim->projectileFrameCount)
  {
    idx = 0U;
  }

  snapshot->pixels = anim->projectileFrames[idx];
  snapshot->sourcePixels = anim->projectileFrames[idx];
  snapshot->width  = anim->projectileWidth;
  snapshot->height = anim->projectileHeight;
  
  // Decompress RLE projectile frame
  uint32_t totalPixels = (uint32_t)anim->projectileWidth * anim->projectileHeight;
  if (totalPixels <= (115U * 70U))
  {
    Battle_DecompressRLE(s_projDecompressBuf, anim->projectileFrames[idx], totalPixels);
    snapshot->pixels = s_projDecompressBuf;
  }
  else
  {
    snapshot->pixels = anim->projectileFrames[idx];
  }

  snapshot->x      = projectile->x;
  snapshot->y      = projectile->y;
  snapshot->flipX  = (projectile->vx < 0) ? 1U : 0U;
  snapshot->valid  = 1U;

  return 1U;
}

static void Battle_DecompressRLE(uint16_t *dest, const uint16_t *src, uint32_t totalPixels)
{
  uint32_t destIdx = 0U;
  uint32_t srcIdx = 0U;

  while (destIdx < totalPixels)
  {
    uint16_t count = src[srcIdx];
    uint16_t color = src[srcIdx + 1U];
    srcIdx += 2U;

    for (uint16_t i = 0U; i < count; i++)
    {
      if (destIdx < totalPixels)
      {
        dest[destIdx] = color;
        destIdx++;
      }
    }
  }
}
