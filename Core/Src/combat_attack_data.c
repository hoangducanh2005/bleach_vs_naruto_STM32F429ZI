#include "combat_attack_data.h"

static const CombatHurtboxProfile s_basicHurtbox = {
    {-14, -52, 28, 52},
    {-15, -50, 30, 50},
    {-16, -52, 32, 52},
    {-14, -50, 28, 50},
    {-15, -52, 30, 52},
    {-16, -54, 32, 54},
    {-16, -50, 32, 50},
};

static const CombatHitboxDef s_basicAttack = {
    2U,
    3U,
    {12, -42, 30, 22},
    6U,
    0U,
    180U,
    100U,
    10,
    0,
    COMBAT_HIT_LEVEL_MID,
    1U,
};

static const CombatHitboxDef s_sasukeAttack = {
    2U,
    3U,
    {12, -42, 28, 22},
    6U,
    0U,
    180U,
    100U,
    10,
    0,
    COMBAT_HIT_LEVEL_MID,
    1U,
};

static const CombatHitboxDef s_sasukeChidori = {
    4U,
    9U,
    {18, -48, 58, 34},
    18U,
    4U,
    420U,
    220U,
    36,
    0,
    COMBAT_HIT_LEVEL_MID,
    1U,
};

static const CombatHitboxDef s_narutoSkill = {
    3U,
    5U,
    {16, -46, 46, 30},
    16U,
    3U,
    360U,
    200U,
    28,
    0,
    COMBAT_HIT_LEVEL_MID,
    1U,
};

static const CombatHitboxDef s_ichigoProjectilePlaceholder = {
    4U,
    7U,
    {24, -48, 54, 28},
    12U,
    3U,
    260U,
    160U,
    18,
    0,
    COMBAT_HIT_LEVEL_PROJECTILE,
    1U,
};

const CombatHurtboxProfile *CombatAttackData_GetHurtboxProfile(
    CombatCharacterId character)
{
  (void)character;
  return &s_basicHurtbox;
}

CombatBox CombatAttackData_GetHurtbox(CombatCharacterId character,
                                      CombatAnimState state)
{
  const CombatHurtboxProfile *profile =
      CombatAttackData_GetHurtboxProfile(character);

  switch (state)
  {
    case COMBAT_ANIM_RUN:
      return profile->run;
    case COMBAT_ANIM_BLOCK:
      return profile->block;
    case COMBAT_ANIM_JUMP:
      return profile->jump;
    case COMBAT_ANIM_ATTACK:
      return profile->attack;
    case COMBAT_ANIM_SKILL:
      return profile->skill;
    case COMBAT_ANIM_HIT:
      return profile->hit;
    case COMBAT_ANIM_DEAD:
      return (CombatBox){0, 0, 0, 0};
    default:
      return profile->idle;
  }
}

const CombatHitboxDef *CombatAttackData_GetHitbox(CombatCharacterId character,
                                                  CombatAnimState state,
                                                  uint8_t frameIndex)
{
  const CombatHitboxDef *hitbox = 0;

  if (state == COMBAT_ANIM_ATTACK)
  {
    hitbox = (character == COMBAT_CHARACTER_SASUKE) ? &s_sasukeAttack
                                                    : &s_basicAttack;
  }
  else if (state == COMBAT_ANIM_SKILL)
  {
    if (character == COMBAT_CHARACTER_SASUKE)
    {
      hitbox = &s_sasukeChidori;
    }
    else if (character == COMBAT_CHARACTER_ICHIGO)
    {
      hitbox = &s_ichigoProjectilePlaceholder;
    }
    else
    {
      hitbox = &s_narutoSkill;
    }
  }

  if (hitbox == 0)
  {
    return 0;
  }

  if ((frameIndex < hitbox->startFrame) || (frameIndex > hitbox->endFrame))
  {
    return 0;
  }

  return hitbox;
}

CombatBox CombatAttackData_GetGuardBox(void)
{
  return (CombatBox){-18, -54, 36, 54};
}
