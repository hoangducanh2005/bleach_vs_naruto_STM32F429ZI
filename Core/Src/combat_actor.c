#include "combat_actor.h"

#if defined(__GNUC__)
#pragma GCC optimize ("O2")
#endif

#include "combat_box.h"
#include "combat_rules.h"
#include "ichigo_moveset.h"
#include "naruto_moveset.h"
#include "sasuke_moveset.h"
#include "vizard_moveset.h"

#define COMBAT_ARENA_MIN_X 32
#define COMBAT_ARENA_MAX_X 288
#define COMBAT_RUN_SPEED 4
#define COMBAT_DASH_SPEED 11
#define COMBAT_JUMP_FORCE -11
#define COMBAT_GRAVITY 1
#define COMBAT_MAX_FALL_SPEED 9
#define COMBAT_MAX_ATTACK_STEP 2U
#define COMBAT_COMBO_WINDOW_MS 520U

static void CombatActor_ApplyPhysics(CombatActor *actor);
static void CombatActor_BeginAttack(CombatActor *actor,
                                    uint8_t attackStep,
                                    uint32_t nowMs);
static void CombatActor_StartDash(CombatActor *actor,
                                  uint8_t inputFlags,
                                  uint32_t nowMs);
static void CombatActor_StartJump(CombatActor *actor, uint32_t nowMs);
static uint8_t CombatActor_GetFrameCount(const CombatActor *actor);
static uint8_t CombatActor_GetLoop(const CombatActor *actor);
static uint16_t CombatActor_GetFrameDuration(const CombatActor *actor,
                                             uint8_t frameIndex);
static uint8_t CombatActor_IsTerminalState(CombatAnimState state);
static uint8_t CombatActor_StateFinished(const CombatActor *actor,
                                         uint32_t nowMs);
static void CombatActor_MapNaruto(const CombatActor *actor,
                                  NarutoMoveState *move);
static void CombatActor_MapSasuke(const CombatActor *actor,
                                  SasukeMoveState *move);
static void CombatActor_MapIchigo(const CombatActor *actor,
                                  IchigoMoveState *move);
static void CombatActor_MapVizard(const CombatActor *actor,
                                  VizardMoveState *move);

void CombatActor_Init(CombatActor *actor,
                      CombatCharacterId character,
                      int16_t x,
                      int16_t y,
                      int8_t facing,
                      uint32_t nowMs)
{
  if (actor == 0)
  {
    return;
  }

  actor->character = character;
  actor->x = x;
  actor->y = y;
  actor->groundY = y;
  actor->facing = (facing < 0) ? -1 : 1;
  actor->vx = 0;
  actor->vy = 0;
  actor->hp = 100U;
  actor->onGround = 1U;
  actor->state = COMBAT_ANIM_IDLE;
  actor->frameIndex = 0U;
  actor->attackStep = 0U;
  actor->queuedAttack = 0U;
  actor->comboNextStep = 0U;
  actor->hitConnected = 0U;
  actor->stunUntilMs = 0U;
  actor->mana = 0U;
  actor->stability = 100;
  actor->invincibleUntilMs = 0U;
  actor->knockdownUntilMs = 0U;
  actor->lastHitTimeMs = 0U;
  actor->stateStartedMs = nowMs;
  actor->comboExpiresMs = 0U;
}

void CombatActor_Update(CombatActor *actor,
                        uint8_t inputFlags,
                        uint32_t nowMs,
                        uint8_t allowInput)
{
  if (actor == 0)
  {
    return;
  }

  if (actor->hp == 0U)
  {
    CombatActor_SetState(actor, COMBAT_ANIM_DEAD, nowMs);
  }

  CombatRules_UpdateKnockdown(actor, nowMs);

  if (nowMs < actor->stunUntilMs)
  {
    actor->vx = (int16_t)(actor->vx / 2);
    CombatActor_ApplyPhysics(actor);
  }
  else if (CombatActor_IsTerminalState(actor->state) != 0U)
  {
    actor->vx = 0;
    actor->vy = 0;
  }
  else if ((actor->state == COMBAT_ANIM_ATTACK) ||
           (actor->state == COMBAT_ANIM_SKILL))
  {
    if ((actor->state == COMBAT_ANIM_ATTACK) &&
        (allowInput != 0U) &&
        ((inputFlags & COMBAT_INPUT_ATTACK) != 0U) &&
        ((actor->attackStep + actor->queuedAttack) < COMBAT_MAX_ATTACK_STEP))
    {
      actor->queuedAttack++;
    }

    CombatActor_ApplyPhysics(actor);
    if (CombatActor_StateFinished(actor, nowMs) != 0U)
    {
      if ((actor->state == COMBAT_ANIM_ATTACK) &&
          (actor->queuedAttack != 0U) &&
          (actor->attackStep < COMBAT_MAX_ATTACK_STEP))
      {
        uint8_t queuedAfterNext = (uint8_t)(actor->queuedAttack - 1U);
        CombatActor_BeginAttack(actor,
                                (uint8_t)(actor->attackStep + 1U),
                                nowMs);
        actor->queuedAttack = queuedAfterNext;
      }
      else
      {
        if (actor->attackStep < COMBAT_MAX_ATTACK_STEP)
        {
          actor->comboNextStep = (uint8_t)(actor->attackStep + 1U);
          actor->comboExpiresMs = nowMs + COMBAT_COMBO_WINDOW_MS;
        }
        else
        {
          actor->comboNextStep = 0U;
          actor->comboExpiresMs = 0U;
        }

        actor->queuedAttack = 0U;
        actor->attackStep = 0U;
        CombatActor_SetState(actor,
                             (actor->onGround != 0U) ? COMBAT_ANIM_IDLE
                                                     : COMBAT_ANIM_JUMP,
                             nowMs);
      }
    }
  }
  else if (actor->state == COMBAT_ANIM_DASH)
  {
    CombatActor_ApplyPhysics(actor);
    if (CombatActor_StateFinished(actor, nowMs) != 0U)
    {
      actor->vx = 0;
      CombatActor_SetState(actor,
                           (actor->onGround != 0U) ? COMBAT_ANIM_IDLE
                                                   : COMBAT_ANIM_JUMP,
                           nowMs);
    }
  }
  else if (actor->state == COMBAT_ANIM_JUMP)
  {
    actor->vx = 0;

    if ((allowInput != 0U) && ((inputFlags & COMBAT_INPUT_LEFT) != 0U))
    {
      actor->facing = -1;
      actor->vx = -COMBAT_RUN_SPEED;
    }
    else if ((allowInput != 0U) && ((inputFlags & COMBAT_INPUT_RIGHT) != 0U))
    {
      actor->facing = 1;
      actor->vx = COMBAT_RUN_SPEED;
    }

    CombatActor_ApplyPhysics(actor);
    if (actor->onGround != 0U)
    {
      CombatActor_SetState(actor,
                           (actor->vx == 0) ? COMBAT_ANIM_IDLE
                                            : COMBAT_ANIM_RUN,
                           nowMs);
    }
  }
  else if (actor->onGround == 0U)
  {
    CombatActor_SetState(actor, COMBAT_ANIM_JUMP, nowMs);
    CombatActor_ApplyPhysics(actor);
  }
  else if (allowInput != 0U)
  {
    actor->vx = 0;

    if ((inputFlags & COMBAT_INPUT_ATTACK) != 0U)
    {
      uint8_t attackStep = 0U;

      if ((actor->comboNextStep != 0U) &&
          (nowMs <= actor->comboExpiresMs))
      {
        attackStep = actor->comboNextStep;
      }

      CombatActor_BeginAttack(actor, attackStep, nowMs);
    }
    else if ((inputFlags & COMBAT_INPUT_DASH) != 0U)
    {
      CombatActor_StartDash(actor, inputFlags, nowMs);
    }
    else if (((inputFlags & COMBAT_INPUT_SKILL) != 0U) && (CombatRules_CanUseSkill(actor) != 0U))
    {
      CombatRules_ConsumeSkillMana(actor);
      CombatActor_SetState(actor, COMBAT_ANIM_SKILL, nowMs);
    }
    else if (((inputFlags & COMBAT_INPUT_JUMP) != 0U) &&
             (actor->onGround != 0U))
    {
      CombatActor_StartJump(actor, nowMs);
    }
    else if ((inputFlags & (COMBAT_INPUT_BLOCK | COMBAT_INPUT_UP)) != 0U)
    {
      if (actor->state != COMBAT_ANIM_BLOCK)
      {
        CombatActor_SetState(actor, COMBAT_ANIM_BLOCK, nowMs);
      }
    }
    else if ((inputFlags & COMBAT_INPUT_LEFT) != 0U)
    {
      actor->facing = -1;
      actor->vx = -COMBAT_RUN_SPEED;
      if (actor->state != COMBAT_ANIM_RUN)
      {
        CombatActor_SetState(actor, COMBAT_ANIM_RUN, nowMs);
      }
    }
    else if ((inputFlags & COMBAT_INPUT_RIGHT) != 0U)
    {
      actor->facing = 1;
      actor->vx = COMBAT_RUN_SPEED;
      if (actor->state != COMBAT_ANIM_RUN)
      {
        CombatActor_SetState(actor, COMBAT_ANIM_RUN, nowMs);
      }
    }
    else if (actor->state != COMBAT_ANIM_IDLE)
    {
      CombatActor_SetState(actor, COMBAT_ANIM_IDLE, nowMs);
    }

    CombatActor_ApplyPhysics(actor);
  }

  uint8_t frameCount = CombatActor_GetFrameCount(actor);
  if (frameCount == 0U)
  {
    actor->frameIndex = 0U;
    return;
  }

  uint32_t elapsedMs = nowMs - actor->stateStartedMs;
  uint8_t nextFrame = 0U;
  uint16_t frameDuration = CombatActor_GetFrameDuration(actor, actor->frameIndex);
  uint32_t rawFrame = (frameDuration == 0U) ? 0U : (elapsedMs / frameDuration);

  if (CombatActor_GetLoop(actor) != 0U)
  {
    nextFrame = (uint8_t)(rawFrame % frameCount);
  }
  else if (rawFrame >= frameCount)
  {
    nextFrame = (uint8_t)(frameCount - 1U);
  }
  else
  {
    nextFrame = (uint8_t)rawFrame;
  }

  actor->frameIndex = nextFrame;
}

void CombatActor_FaceToward(CombatActor *actor, const CombatActor *target)
{
  if ((actor == 0) || (target == 0))
  {
    return;
  }

  if ((actor->state == COMBAT_ANIM_ATTACK) ||
      (actor->state == COMBAT_ANIM_SKILL) ||
      (actor->state == COMBAT_ANIM_DASH) ||
      (actor->state == COMBAT_ANIM_HIT) ||
      (actor->state == COMBAT_ANIM_DEAD))
  {
    return;
  }

  actor->facing = (target->x < actor->x) ? -1 : 1;
}


static void CombatActor_BeginAttack(CombatActor *actor,
                                    uint8_t attackStep,
                                    uint32_t nowMs)
{
  if (actor == 0)
  {
    return;
  }

  actor->state = COMBAT_ANIM_ATTACK;
  actor->frameIndex = 0U;
  actor->attackStep = (attackStep > COMBAT_MAX_ATTACK_STEP)
                          ? COMBAT_MAX_ATTACK_STEP
                          : attackStep;
  actor->queuedAttack = 0U;
  actor->comboNextStep = 0U;
  actor->comboExpiresMs = 0U;
  actor->hitConnected = 0U;
  actor->stateStartedMs = nowMs;
}

static void CombatActor_ApplyPhysics(CombatActor *actor)
{
  if (actor == 0)
  {
    return;
  }

  actor->x = (int16_t)(actor->x + actor->vx);
  actor->x = CombatBox_ClampI16(actor->x, COMBAT_ARENA_MIN_X, COMBAT_ARENA_MAX_X);

  if (actor->onGround == 0U)
  {
    actor->vy = (int16_t)(actor->vy + COMBAT_GRAVITY);
    if (actor->vy > COMBAT_MAX_FALL_SPEED)
    {
      actor->vy = COMBAT_MAX_FALL_SPEED;
    }

    actor->y = (int16_t)(actor->y + actor->vy);
    if (actor->y >= actor->groundY)
    {
      actor->y = actor->groundY;
      actor->vy = 0;
      actor->onGround = 1U;
    }
  }
}

static void CombatActor_StartJump(CombatActor *actor, uint32_t nowMs)
{
  if (actor == 0)
  {
    return;
  }

  actor->onGround = 0U;
  actor->vy = COMBAT_JUMP_FORCE;
  CombatActor_SetState(actor, COMBAT_ANIM_JUMP, nowMs);
}

static void CombatActor_StartDash(CombatActor *actor,
                                  uint8_t inputFlags,
                                  uint32_t nowMs)
{
  if (actor == 0)
  {
    return;
  }

  if ((inputFlags & COMBAT_INPUT_LEFT) != 0U)
  {
    actor->facing = -1;
  }
  else if ((inputFlags & COMBAT_INPUT_RIGHT) != 0U)
  {
    actor->facing = 1;
  }

  actor->vx = (actor->facing < 0) ? -COMBAT_DASH_SPEED : COMBAT_DASH_SPEED;
  actor->vy = 0;
  CombatActor_SetState(actor, COMBAT_ANIM_DASH, nowMs);
}

CombatBox CombatActor_GetHurtboxWorld(const CombatActor *actor)
{
  if (actor == 0)
  {
    return (CombatBox){0, 0, 0, 0};
  }

  return CombatBox_ToWorld(
      CombatAttackData_GetHurtbox(actor->character, actor->state),
      actor->x,
      actor->y,
      actor->facing);
}

CombatBox CombatActor_GetHitboxWorld(const CombatActor *actor,
                                     const CombatHitboxDef **hitboxOut)
{
  if (hitboxOut != 0)
  {
    *hitboxOut = 0;
  }

  if (actor == 0)
  {
    return (CombatBox){0, 0, 0, 0};
  }

  const CombatHitboxDef *hitbox = CombatAttackData_GetHitbox(
      actor->character,
      actor->state,
      actor->attackStep,
      actor->frameIndex);

  if (hitboxOut != 0)
  {
    *hitboxOut = hitbox;
  }

  if (hitbox == 0)
  {
    return (CombatBox){0, 0, 0, 0};
  }

  return CombatBox_ToWorld(hitbox->box, actor->x, actor->y, actor->facing);
}

uint8_t CombatActor_IsBlockingFront(const CombatActor *target,
                                    const CombatActor *attacker)
{
  if ((target == 0) || (attacker == 0) ||
      (target->state != COMBAT_ANIM_BLOCK))
  {
    return 0U;
  }

  if ((attacker->x > target->x) && (target->facing < 0))
  {
    return 0U;
  }

  if ((attacker->x < target->x) && (target->facing > 0))
  {
    return 0U;
  }

  return 1U;
}

uint8_t CombatActor_GetFrame(const CombatActor *actor, CombatFrameView *outFrame)
{
  if ((actor == 0) || (outFrame == 0))
  {
    return 0U;
  }

  if (actor->character == COMBAT_CHARACTER_SASUKE)
  {
    SasukeMoveState move;
    CombatActor_MapSasuke(actor, &move);
    const SasukeMoveAnimation *anim = &sasuke_move_animations[move];
    const SasukeMoveFrame *frame = &anim->frames[actor->frameIndex];
    outFrame->pixels = frame->pixels;
    outFrame->width = frame->width;
    outFrame->height = frame->height;
    outFrame->pivotX = frame->pivotX;
    outFrame->pivotY = frame->pivotY;
    outFrame->durationMs = frame->durationMs;
    return 1U;
  }

  if (actor->character == COMBAT_CHARACTER_ICHIGO)
  {
    IchigoMoveState move;
    CombatActor_MapIchigo(actor, &move);
    const IchigoMoveAnimation *anim = &ichigo_move_animations[move];
    const IchigoMoveFrame *frame = &anim->frames[actor->frameIndex];
    outFrame->pixels = frame->pixels;
    outFrame->width = frame->width;
    outFrame->height = frame->height;
    outFrame->pivotX = frame->pivotX;
    outFrame->pivotY = frame->pivotY;
    outFrame->durationMs = frame->durationMs;
    return 1U;
  }

  if (actor->character == COMBAT_CHARACTER_VIZARD_ICHIGO)
  {
    if ((actor->state == COMBAT_ANIM_HIT) ||
        (actor->state == COMBAT_ANIM_DEAD))
    {
      IchigoMoveState move;
      CombatActor_MapIchigo(actor, &move);
      const IchigoMoveAnimation *anim = &ichigo_move_animations[move];
      const IchigoMoveFrame *frame = &anim->frames[actor->frameIndex];
      outFrame->pixels = frame->pixels;
      outFrame->width = frame->width;
      outFrame->height = frame->height;
      outFrame->pivotX = frame->pivotX;
      outFrame->pivotY = frame->pivotY;
      outFrame->durationMs = frame->durationMs;
      return 1U;
    }

    VizardMoveState move;
    CombatActor_MapVizard(actor, &move);
    const VizardMoveAnimation *anim = &vizard_move_animations[move];
    const VizardMoveFrame *frame = &anim->frames[actor->frameIndex];
    outFrame->pixels = frame->pixels;
    outFrame->width = frame->width;
    outFrame->height = frame->height;
    outFrame->pivotX = frame->pivotX;
    outFrame->pivotY = frame->pivotY;
    outFrame->durationMs = frame->durationMs;
    return 1U;
  }

  NarutoMoveState move;
  CombatActor_MapNaruto(actor, &move);
  const NarutoMoveAnimation *anim = &naruto_move_animations[move];
  const NarutoMoveFrame *frame = &anim->frames[actor->frameIndex];
  outFrame->pixels = frame->pixels;
  outFrame->width = frame->width;
  outFrame->height = frame->height;
  outFrame->pivotX = frame->pivotX;
  outFrame->pivotY = frame->pivotY;
  outFrame->durationMs = frame->durationMs;
  return 1U;
}

uint8_t CombatActor_IsActionLocked(const CombatActor *actor, uint32_t nowMs)
{
  if (actor == 0)
  {
    return 0U;
  }

  return ((nowMs < actor->stunUntilMs) ||
          (nowMs < actor->knockdownUntilMs) ||
          (actor->state == COMBAT_ANIM_ATTACK) ||
          (actor->state == COMBAT_ANIM_SKILL) ||
          (actor->state == COMBAT_ANIM_DASH) ||
          (actor->state == COMBAT_ANIM_HIT) ||
          (actor->state == COMBAT_ANIM_DEAD))
             ? 1U
             : 0U;
}

void CombatActor_SetState(CombatActor *actor,
                          CombatAnimState state,
                          uint32_t nowMs)
{
  if ((actor == 0) || (actor->state == state))
  {
    return;
  }

    actor->state = state;
  actor->frameIndex = 0U;
  if (state != COMBAT_ANIM_ATTACK)
  {
    actor->attackStep = 0U;
    actor->queuedAttack = 0U;
    if ((state == COMBAT_ANIM_SKILL) ||
        (state == COMBAT_ANIM_DASH) ||
        (state == COMBAT_ANIM_JUMP) ||
        (state == COMBAT_ANIM_HIT) ||
        (state == COMBAT_ANIM_DEAD))
    {
      actor->comboNextStep = 0U;
      actor->comboExpiresMs = 0U;
    }
  }
  actor->hitConnected = 0U;
  actor->stateStartedMs = nowMs;
}

static uint8_t CombatActor_GetFrameCount(const CombatActor *actor)
{
  if (actor->character == COMBAT_CHARACTER_SASUKE)
  {
    SasukeMoveState move;
    CombatActor_MapSasuke(actor, &move);
    return sasuke_move_animations[move].frameCount;
  }

  if (actor->character == COMBAT_CHARACTER_ICHIGO)
  {
    IchigoMoveState move;
    CombatActor_MapIchigo(actor, &move);
    return ichigo_move_animations[move].frameCount;
  }

  if (actor->character == COMBAT_CHARACTER_VIZARD_ICHIGO)
  {
    if ((actor->state == COMBAT_ANIM_HIT) ||
        (actor->state == COMBAT_ANIM_DEAD))
    {
      IchigoMoveState move;
      CombatActor_MapIchigo(actor, &move);
      return ichigo_move_animations[move].frameCount;
    }

    VizardMoveState move;
    CombatActor_MapVizard(actor, &move);
    return vizard_move_animations[move].frameCount;
  }

  NarutoMoveState move;
  CombatActor_MapNaruto(actor, &move);
  return naruto_move_animations[move].frameCount;
}

static uint8_t CombatActor_GetLoop(const CombatActor *actor)
{
  if (actor->character == COMBAT_CHARACTER_SASUKE)
  {
    SasukeMoveState move;
    CombatActor_MapSasuke(actor, &move);
    return sasuke_move_animations[move].loop;
  }

  if (actor->character == COMBAT_CHARACTER_ICHIGO)
  {
    IchigoMoveState move;
    CombatActor_MapIchigo(actor, &move);
    return ichigo_move_animations[move].loop;
  }

  if (actor->character == COMBAT_CHARACTER_VIZARD_ICHIGO)
  {
    if ((actor->state == COMBAT_ANIM_HIT) ||
        (actor->state == COMBAT_ANIM_DEAD))
    {
      IchigoMoveState move;
      CombatActor_MapIchigo(actor, &move);
      return ichigo_move_animations[move].loop;
    }

    VizardMoveState move;
    CombatActor_MapVizard(actor, &move);
    return vizard_move_animations[move].loop;
  }

  NarutoMoveState move;
  CombatActor_MapNaruto(actor, &move);
  return naruto_move_animations[move].loop;
}

static uint16_t CombatActor_GetFrameDuration(const CombatActor *actor,
                                             uint8_t frameIndex)
{
  CombatFrameView frame;
  CombatActor temp = *actor;
  temp.frameIndex = frameIndex;

  if (CombatActor_GetFrame(&temp, &frame) == 0U)
  {
    return 80U;
  }

  return frame.durationMs;
}

static uint8_t CombatActor_IsTerminalState(CombatAnimState state)
{
  return (state == COMBAT_ANIM_DEAD) ? 1U : 0U;
}

static uint8_t CombatActor_StateFinished(const CombatActor *actor,
                                         uint32_t nowMs)
{
  uint8_t frameCount = CombatActor_GetFrameCount(actor);
  uint32_t totalMs = 0U;

  for (uint8_t i = 0U; i < frameCount; i++)
  {
    totalMs += CombatActor_GetFrameDuration(actor, i);
  }

  return ((nowMs - actor->stateStartedMs) >= totalMs) ? 1U : 0U;
}

static void CombatActor_MapNaruto(const CombatActor *actor,
                                  NarutoMoveState *move)
{
  switch (actor->state)
  {
    case COMBAT_ANIM_RUN:
      *move = NARUTO_MOVE_RUN;
      break;
    case COMBAT_ANIM_DASH:
      *move = NARUTO_MOVE_DASH;
      break;
    case COMBAT_ANIM_BLOCK:
      *move = NARUTO_MOVE_BLOCK;
      break;
    case COMBAT_ANIM_JUMP:
      *move = NARUTO_MOVE_JUMP;
      break;
    case COMBAT_ANIM_ATTACK:
      if (actor->attackStep == 1U)
      {
        *move = NARUTO_MOVE_ATTACK2;
      }
      else if (actor->attackStep >= 2U)
      {
        *move = NARUTO_MOVE_ATTACK3;
      }
      else
      {
        *move = NARUTO_MOVE_ATTACK_LIGHT;
      }
      break;
    case COMBAT_ANIM_SKILL:
      *move = NARUTO_MOVE_SKILL;
      break;
    case COMBAT_ANIM_HIT:
      *move = NARUTO_MOVE_HIT;
      break;
    case COMBAT_ANIM_DEAD:
      *move = NARUTO_MOVE_DEAD;
      break;
    default:
      *move = NARUTO_MOVE_IDLE;
      break;
  }
}

static void CombatActor_MapSasuke(const CombatActor *actor,
                                  SasukeMoveState *move)
{
  switch (actor->state)
  {
    case COMBAT_ANIM_RUN:
      *move = SASUKE_MOVE_RUN;
      break;
    case COMBAT_ANIM_DASH:
      *move = SASUKE_MOVE_DASH;
      break;
    case COMBAT_ANIM_BLOCK:
      *move = SASUKE_MOVE_BLOCK;
      break;
    case COMBAT_ANIM_JUMP:
      *move = SASUKE_MOVE_JUMP_AIR;
      break;
    case COMBAT_ANIM_ATTACK:
      if (actor->attackStep == 1U)
      {
        *move = SASUKE_MOVE_ATTACK2;
      }
      else if (actor->attackStep >= 2U)
      {
        *move = SASUKE_MOVE_ATTACK3;
      }
      else
      {
        *move = SASUKE_MOVE_ATTACK1;
      }
      break;
    case COMBAT_ANIM_SKILL:
      *move = SASUKE_MOVE_SKILL;
      break;
    case COMBAT_ANIM_HIT:
      *move = SASUKE_MOVE_HIT;
      break;
    case COMBAT_ANIM_DEAD:
      *move = SASUKE_MOVE_DEAD;
      break;
    default:
      *move = SASUKE_MOVE_IDLE;
      break;
  }
}

static void CombatActor_MapIchigo(const CombatActor *actor,
                                  IchigoMoveState *move)
{
  switch (actor->state)
  {
    case COMBAT_ANIM_RUN:
      *move = ICHIGO_MOVE_RUN;
      break;
    case COMBAT_ANIM_DASH:
      *move = ICHIGO_MOVE_DASH;
      break;
    case COMBAT_ANIM_BLOCK:
      *move = ICHIGO_MOVE_BLOCK;
      break;
    case COMBAT_ANIM_JUMP:
      *move = ICHIGO_MOVE_JUMP;
      break;
    case COMBAT_ANIM_ATTACK:
      if (actor->attackStep == 1U)
      {
        *move = ICHIGO_MOVE_ATTACK2;
      }
      else if (actor->attackStep >= 2U)
      {
        *move = ICHIGO_MOVE_ATTACK3;
      }
      else
      {
        *move = ICHIGO_MOVE_ATTACK_LIGHT;
      }
      break;
    case COMBAT_ANIM_SKILL:
      *move = ICHIGO_MOVE_SKILL;
      break;
    case COMBAT_ANIM_HIT:
      *move = ICHIGO_MOVE_HIT;
      break;
    case COMBAT_ANIM_DEAD:
      *move = ICHIGO_MOVE_DEAD;
      break;
    default:
      *move = ICHIGO_MOVE_IDLE;
      break;
  }
}

static void CombatActor_MapVizard(const CombatActor *actor,
                                  VizardMoveState *move)
{
  switch (actor->state)
  {
    case COMBAT_ANIM_RUN:
      *move = VIZARD_MOVE_RUN;
      break;
    case COMBAT_ANIM_DASH:
      *move = VIZARD_MOVE_DASH;
      break;
    case COMBAT_ANIM_BLOCK:
      *move = VIZARD_MOVE_BLOCK;
      break;
    case COMBAT_ANIM_JUMP:
      *move = VIZARD_MOVE_JUMP;
      break;
    case COMBAT_ANIM_ATTACK:
      if (actor->attackStep == 1U)
      {
        *move = VIZARD_MOVE_ATTACK2;
      }
      else if (actor->attackStep >= 2U)
      {
        *move = VIZARD_MOVE_ATTACK3;
      }
      else
      {
        *move = VIZARD_MOVE_ATTACK_LIGHT;
      }
      break;
    case COMBAT_ANIM_SKILL:
      *move = VIZARD_MOVE_SKILL;
      break;
    default:
      *move = VIZARD_MOVE_IDLE;
      break;
  }
}
