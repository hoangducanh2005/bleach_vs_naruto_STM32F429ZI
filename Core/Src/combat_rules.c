#include "combat_rules.h"
#include <stdint.h>

// Hàm SetState nội bộ của combat_actor
extern void CombatActor_SetState(CombatActor *actor, CombatAnimState state, uint32_t nowMs);

uint8_t CombatRules_IsInvincible(const CombatActor *actor, uint32_t nowMs)
{
  if (actor == 0)
  {
    return 0U;
  }
  return (nowMs < actor->invincibleUntilMs) ? 1U : 0U;
}

uint8_t CombatRules_CanUseSkill(const CombatActor *actor)
{
  if (actor == 0)
  {
    return 0U;
  }
  // Kỹ năng cần ít nhất 50 Mana
  return (actor->mana >= COMBAT_SKILL_MANA_COST) ? 1U : 0U;
}

void CombatRules_ConsumeSkillMana(CombatActor *actor)
{
  if (actor == 0)
  {
    return;
  }
  if (actor->mana >= COMBAT_SKILL_MANA_COST)
  {
    actor->mana -= COMBAT_SKILL_MANA_COST;
  }
  else
  {
    actor->mana = 0U;
  }
}

void CombatRules_ProcessHit(CombatActor *attacker, CombatActor *target, const CombatHitboxDef *hitbox, uint32_t nowMs)
{
  if ((target == 0) || (attacker == 0) || (hitbox == 0) ||
      (target->state == COMBAT_ANIM_DEAD))
  {
    return;
  }

  // 1. Kiểm tra bất tử
  if (CombatRules_IsInvincible(target, nowMs) != 0U)
  {
    return;
  }

  // Ghi nhận mốc thời gian bị đánh trúng mới nhất
  target->lastHitTimeMs = nowMs;

  // 2. Kiểm tra đỡ đòn
  uint8_t blocked = CombatActor_IsBlockingFront(target, attacker);

  // 3. Tính toán sát thương
  uint8_t damage = blocked ? hitbox->blockDamage : hitbox->damage;
  if (target->hp > damage)
  {
    target->hp = (uint16_t)(target->hp - damage);
  }
  else
  {
    target->hp = 0U;
  }

  // HP về 0: Chuyển thẳng sang DEAD
  if (target->hp == 0U)
  {
    CombatActor_SetState(target, COMBAT_ANIM_DEAD, nowMs);
    return;
  }

  // 4. Hồi Mana cho người tấn công
  // Đòn tấn công thường mới hồi mana
  if (attacker->state == COMBAT_ANIM_ATTACK)
  {
    uint16_t manaGain = blocked ? COMBAT_BLOCK_MANA_GAIN : COMBAT_LIGHT_MANA_GAIN;
    attacker->mana += manaGain;
    if (attacker->mana > COMBAT_MAX_MANA)
    {
      attacker->mana = COMBAT_MAX_MANA;
    }
  }

  // 5. Xử lý điểm kiên định (stability) & ngã sàn (knockdown)
  if ((attacker->state == COMBAT_ANIM_ATTACK) && (hitbox->level != COMBAT_HIT_LEVEL_PROJECTILE))
  {
    // Đòn thường: không đỡ trừ 20, đỡ trừ 0
    if (blocked == 0U)
    {
      target->stability -= COMBAT_LIGHT_STABILITY_DECAY;
    }
  }
  else if ((attacker->state == COMBAT_ANIM_SKILL) || (hitbox->level == COMBAT_HIT_LEVEL_PROJECTILE))
  {
    // Chiêu đặc biệt hoặc Projectile: không đỡ trừ 100, đỡ trừ 20
    if (blocked == 0U)
    {
      target->stability -= 100;
    }
    else
    {
      target->stability -= COMBAT_BLOCK_SKILL_DECAY;
    }
  }

  // Nếu điểm kiên định về 0: ngã sàn
  if (target->stability <= 0)
  {
    target->stability = 0;
    CombatActor_SetState(target, COMBAT_ANIM_DEAD, nowMs);
    target->knockdownUntilMs = nowMs + COMBAT_KNOCKDOWN_DURATION_MS;
    target->invincibleUntilMs = nowMs + COMBAT_KNOCKDOWN_DURATION_MS;
  }
  else
  {
    // Chưa ngã sàn: set trạng thái đơ đòn (HIT/BLOCK) và áp lực đẩy lùi
    // Áp dụng bất tử I-frames 500ms sau mỗi lần bị trúng đòn
    if (blocked == 0U)
    {
      target->invincibleUntilMs = nowMs + COMBAT_INVINCIBLE_DURATION_MS;
    }

    if (blocked != 0U)
    {
      CombatActor_SetState(target, COMBAT_ANIM_BLOCK, nowMs);
      target->stunUntilMs = nowMs + hitbox->blockStunMs;
      target->vx = (attacker->x < target->x) ? (int16_t)(hitbox->knockbackX / 2)
                                             : (int16_t)(-hitbox->knockbackX / 2);
    }
    else
    {
      CombatActor_SetState(target, COMBAT_ANIM_HIT, nowMs);
      target->stunUntilMs = nowMs + hitbox->hitStunMs;
      target->vx = (attacker->x < target->x) ? hitbox->knockbackX
                                             : (int16_t)(-hitbox->knockbackX);
    }
  }
}

void CombatRules_UpdateKnockdown(CombatActor *actor, uint32_t nowMs)
{
  if (actor == 0)
  {
    return;
  }

  // 1. Tự động hồi phục điểm kiên định khi không bị tấn công trong 1.5 giây
  if ((actor->state != COMBAT_ANIM_DEAD) && (actor->stability < COMBAT_MAX_STABILITY))
  {
    if ((nowMs - actor->lastHitTimeMs) >= COMBAT_STABILITY_RECOVERY_MS)
    {
      actor->stability = COMBAT_MAX_STABILITY;
    }
  }

  // 2. Quản lý trạng thái ngã sàn và hồi phục
  // Nhân vật ở trạng thái DEAD nhưng HP vẫn > 0 thì chính là đang nằm sàn (knockdown)
  if ((actor->state == COMBAT_ANIM_DEAD) && (actor->hp > 0U))
  {
    if (nowMs >= actor->knockdownUntilMs)
    {
      // Đứng dậy: về IDLE, hồi phục giáp
      CombatActor_SetState(actor, COMBAT_ANIM_IDLE, nowMs);
      actor->stability = COMBAT_MAX_STABILITY;
      actor->knockdownUntilMs = 0U; // Reset
    }
  }
}
