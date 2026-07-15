#include "combat_attack_data.h"

static const CombatHurtboxProfile s_basicHurtbox = {
    {-14, -52, 28, 52},
    {-15, -50, 30, 50},
    {-16, -48, 32, 48},
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

static const CombatHitboxDef s_basicAttack2 = {
    1U,
    2U,
    {10, -46, 38, 26},
    9U,
    0U,
    230U,
    130U,
    16,
    0,
    COMBAT_HIT_LEVEL_MID,
    1U,
};

static const CombatHitboxDef s_basicAttack3 = {
    2U,
    3U,
    {14, -48, 44, 30},
    12U,
    0U,
    300U,
    160U,
    24,
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

static const CombatHitboxDef s_sasukeAttack2 = {
    2U,
    6U,
    {10, -46, 38, 26},
    9U,
    0U,
    230U,
    130U,
    16,
    0,
    COMBAT_HIT_LEVEL_MID,
    1U,
};

static const CombatHitboxDef s_sasukeAttack3 = {
    3U,
    5U,
    {14, -48, 44, 30},
    12U,
    0U,
    300U,
    160U,
    24,
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

static const CombatHitboxDef s_vizardAuraAttack = {
    2U,
    5U,
    {14, -50, 52, 34},
    14U,
    3U,
    360U,
    180U,
    28,
    0,
    COMBAT_HIT_LEVEL_MID,
    1U,
};

static const CombatHitboxDef s_narutoDirectionAttack = {
    9U,
    13U,
    {18, -48, 58, 34},
    18U,
    4U,
    420U,
    220U,
    34,
    0,
    COMBAT_HIT_LEVEL_MID,
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
    case COMBAT_ANIM_DASH:
      return profile->dash;
    case COMBAT_ANIM_BLOCK:
      return profile->block;
    case COMBAT_ANIM_JUMP:
      return profile->jump;
    case COMBAT_ANIM_ATTACK:
      return profile->attack;
    case COMBAT_ANIM_SKILL:
    case COMBAT_ANIM_SPECIAL:
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
                                                  uint8_t attackStep,
                                                  uint8_t frameIndex)
{
  const CombatHitboxDef *hitbox = 0;

  if (state == COMBAT_ANIM_ATTACK)
  {
    if (character == COMBAT_CHARACTER_SASUKE)
    {
      if (attackStep == 1U)
      {
        hitbox = &s_sasukeAttack2;
      }
      else if (attackStep >= 2U)
      {
        hitbox = &s_sasukeAttack3;
      }
      else
      {
        hitbox = &s_sasukeAttack;
      }
    }
    else if (attackStep == 1U)
    {
      hitbox = &s_basicAttack2;
    }
    else if (attackStep >= 2U)
    {
      hitbox = &s_basicAttack3;
    }
    else
    {
      hitbox = &s_basicAttack;
    }
  }
  else if (state == COMBAT_ANIM_SKILL)
  {
    if (character == COMBAT_CHARACTER_SASUKE)
    {
      hitbox = &s_sasukeChidori;
    }
    else if ((character == COMBAT_CHARACTER_ICHIGO) ||
             (character == COMBAT_CHARACTER_VIZARD_ICHIGO))
    {
      hitbox = 0;
    }
    else
    {
      hitbox = &s_narutoSkill;
    }
  }
  else if ((state == COMBAT_ANIM_SPECIAL) &&
           (character == COMBAT_CHARACTER_VIZARD_ICHIGO))
  {
    hitbox = &s_vizardAuraAttack;
  }
  else if ((state == COMBAT_ANIM_SPECIAL) &&
           (character == COMBAT_CHARACTER_NARUTO))
  {
    hitbox = &s_narutoDirectionAttack;
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
