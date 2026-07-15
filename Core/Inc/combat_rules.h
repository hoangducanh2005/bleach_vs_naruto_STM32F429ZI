#ifndef __COMBAT_RULES_H
#define __COMBAT_RULES_H

#include "combat_actor.h"

// Các hằng số định nghĩa luật chơi
#define COMBAT_MAX_MANA               100U
#define COMBAT_SKILL_MANA_COST        50U
#define COMBAT_LIGHT_MANA_GAIN        10U
#define COMBAT_BLOCK_MANA_GAIN        1U
#define COMBAT_MAX_STABILITY          100
#define COMBAT_LIGHT_STABILITY_DECAY  20
#define COMBAT_BLOCK_SKILL_DECAY      20
#define COMBAT_STABILITY_RECOVERY_MS  1500U
#define COMBAT_INVINCIBLE_DURATION_MS 500U
#define COMBAT_KNOCKDOWN_DURATION_MS  1500U

// Kiểm tra xem nhân vật có đang trong trạng thái bất tử hay không
uint8_t CombatRules_IsInvincible(const CombatActor *actor, uint32_t nowMs);

// Kiểm tra xem nhân vật có đủ mana và điều kiện để tung Skill hay không
uint8_t CombatRules_CanUseSkill(const CombatActor *actor);

// Khấu trừ mana khi tung Skill
void CombatRules_ConsumeSkillMana(CombatActor *actor);

// Xử lý khi đòn đánh trúng mục tiêu (tính sát thương, đỡ đòn, hồi mana, trừ stability)
void CombatRules_ProcessHit(CombatActor *attacker, CombatActor *target, const CombatHitboxDef *hitbox, uint32_t nowMs);

// Cập nhật trạng thái tự động hồi phục kiên định và hồi phục nằm sàn
void CombatRules_UpdateKnockdown(CombatActor *actor, uint32_t nowMs);

#endif /* __COMBAT_RULES_H */
