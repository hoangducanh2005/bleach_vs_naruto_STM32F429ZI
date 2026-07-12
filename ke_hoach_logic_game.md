# Kế hoạch Hiện thực hóa Logic Game (HP, Mana, Block, Stability & Knockdown)

Tài liệu này hướng dẫn chi tiết các bước cần làm theo thứ tự tuần tự để triển khai đầy đủ luật chơi và các cơ chế đối kháng nâng cao sử dụng mô hình **Tách logic game luật chơi ra file mới và chỉnh sửa nhẹ cấu trúc nhân vật**.

---

## 1. Mô tả chi tiết Cơ chế & Chỉ số Gameplay cần đạt được

1.  **Hệ thống HP thực tế:** Trừ HP đúng lượng sát thương từ hitbox; hỗ trợ giảm sát thương khi đỡ đòn thành công.
2.  **Cơ chế Mana (Năng lượng) - Hệ thống 2 Thanh (50 x 2):**
    *   Giới hạn tối đa: `100 Mana`.
    *   Tích lũy Mana:
        *   Đánh thường trúng đối phương (không đỡ): Tăng **`10 Mana`** (cần 5 hit trúng để đủ 1 chiêu).
        *   Đánh trúng khi đối phương đỡ (`Block`): Chỉ tăng **`1 Mana`**.
        *   Đánh hụt: Không tăng mana.
    *   Tiêu hao Mana: Kỹ năng đặc biệt (`SKILL`) tốn **`50 Mana`** (cho phép lưu trữ tối đa 2 lần dùng chiêu khi đầy thanh 100 Mana).
3.  **Cơ chế Đỡ đòn (Block):**
    *   Giảm `80%` sát thương nhận vào khi ở trạng thái Block.
4.  **Hệ thống Bất tử (Invincible I-Frames):**
    *   Bảo vệ nhân vật trong `500 ms` sau khi bị trúng đòn để chống việc dính combo liên tục không thể thoát.
5.  **Cơ chế Điểm kiên định & Ngã sàn (Stability & Knockdown):**
    *   Mỗi nhân vật có tối đa `100 điểm kiên định`.
    *   **Quy tắc giảm điểm kiên định:**
        *   **Khi bị đánh bởi đòn thường:** Giảm **`20 điểm`** kiên định nếu không đỡ đòn. Nếu đối phương đang đỡ đòn (`Block`), đòn thường **không làm giảm** điểm kiên định (giảm `0 điểm`).
        *   **Khi bị đánh bởi kỹ năng đặc biệt:** Giảm thẳng **`100 điểm`** (gây ngã sàn ngay lập tức) nếu không đỡ đòn. Nếu đối phương đang đỡ đòn (`Block`), kỹ năng đặc biệt làm giảm **`20 điểm`** kiên định.
    *   **Tự động hồi phục điểm kiên định:** Nếu nhân vật không bị trúng đòn trong vòng **1.5 giây (1500 ms)**, điểm kiên định sẽ tự động hồi phục về mức tối đa `100` ngay lập tức.
    *   Khi điểm kiên định giảm về `0`, nhân vật bị ngã sàn (dùng frame cuối của hoạt ảnh `DEAD` nhưng không làm HP về 0).
    *   Khi nằm sàn, nhân vật được bất tử hoàn toàn trong `600 ms`.
    *   Hết thời gian nằm sàn, nhân vật đứng dậy (`IDLE`) và phục hồi `100 điểm kiên định`.

---

## 2. Danh sách công việc cần thực hiện theo thứ tự

### Bước 1: Nâng cấp cấu trúc dữ liệu nhân vật (`combat_actor.h` & `combat_actor.c`)
1.  **Chỉnh sửa file:** [combat_actor.h](file:///d:/HUST/2025_2/Nh%C3%BAng/PROJECT_12_7/bleach_vs_naruto_STM32F429ZI/Core/Inc/combat_actor.h)
    Bổ sung các trường quản lý trạng thái Mana, Giáp (Stability) và Bất tử vào struct `CombatActor`:
    ```c
    typedef struct
    {
      // ... (Giữ nguyên các trường cũ) ...
      uint16_t mana;              // Năng lượng (0 -> 100)
      int16_t stability;          // Điểm kiên định (0 -> 100)
      uint32_t invincibleUntilMs; // Mốc thời gian kết thúc trạng thái bất tử (ms)
      uint32_t knockdownUntilMs;  // Mốc thời gian kết thúc nằm sàn (ms)
      uint32_t lastHitTimeMs;     // Mốc thời gian cuối cùng bị trúng đòn (ms)
    } CombatActor;
    ```

2.  **Chỉnh sửa file:** [combat_actor.c](file:///d:/HUST/2025_2/Nh%C3%BAng/PROJECT_12_7/bleach_vs_naruto_STM32F429ZI/Core/Src/combat_actor.c)
    Cập nhật hàm `CombatActor_Init` để khởi tạo giá trị ban đầu cho các trường mới:
    ```c
    actor->mana = 0U;
    actor->stability = 100;
    actor->invincibleUntilMs = 0U;
    actor->knockdownUntilMs = 0U;
    actor->lastHitTimeMs = 0U;
    ```

---

### Bước 2: Tạo module quản lý luật chơi mới (`combat_rules.h` & `combat_rules.c`)
1.  **Tạo file mới:** `Core/Inc/combat_rules.h`
    Khai báo các hằng số quy chuẩn luật chơi và nguyên mẫu hàm:
    ```c
    #ifndef __COMBAT_RULES_H
    #define __COMBAT_RULES_H

    #include "combat_actor.h"

    #define COMBAT_MAX_MANA               100U
    #define COMBAT_SKILL_MANA_COST        50U
    #define COMBAT_LIGHT_MANA_GAIN        10U
    #define COMBAT_BLOCK_MANA_GAIN        1U
    #define COMBAT_MAX_STABILITY          100
    #define COMBAT_LIGHT_STABILITY_DECAY  20
    #define COMBAT_BLOCK_SKILL_DECAY      20
    #define COMBAT_STABILITY_RECOVERY_MS  1500U
    #define COMBAT_INVINCIBLE_DURATION_MS 500U
    #define COMBAT_KNOCKDOWN_DURATION_MS  600U

    uint8_t CombatRules_IsInvincible(const CombatActor *actor, uint32_t nowMs);
    uint8_t CombatRules_CanUseSkill(const CombatActor *actor);
    void CombatRules_ConsumeSkillMana(CombatActor *actor);
    void CombatRules_ProcessHit(CombatActor *attacker, CombatActor *target, const CombatHitboxDef *hitbox, uint32_t nowMs);
    void CombatRules_UpdateKnockdown(CombatActor *actor, uint32_t nowMs);

    #endif /* __COMBAT_RULES_H */
    ```

2.  **Tạo file mới:** `Core/Src/combat_rules.c`
    Hiện thực hóa logic các hàm:
    *   `CombatRules_IsInvincible`: Trả về `1` nếu `nowMs < actor->invincibleUntilMs`.
    *   `CombatRules_CanUseSkill`: Trả về `1` nếu `actor->mana >= COMBAT_SKILL_MANA_COST` và không bị khóa hành động.
    *   `CombatRules_ConsumeSkillMana`: Khấu trừ `COMBAT_SKILL_MANA_COST` khỏi mana nhân vật.
    *   `CombatRules_ProcessHit`:
        *   Bỏ qua nếu target đang bất tử (`CombatRules_IsInvincible`).
        *   Ghi nhận thời điểm trúng đòn: `target->lastHitTimeMs = nowMs`.
        *   Kiểm tra đỡ đòn hướng trước mặt (`CombatActor_IsBlockingFront`).
        *   Trừ HP dựa theo kết quả đỡ đòn (Đỡ: nhận `blockDamage`, Không đỡ: nhận `damage`).
        *   Hồi Mana cho `attacker` từ đòn đánh thường: Trúng đích hồi `10 mana`, bị block hồi `1 mana`.
        *   Trừ Stability của target:
            *   Đòn đánh thường: Không đỡ mất `20 điểm`, Đỡ mất `0 điểm`.
            *   Kỹ năng đặc biệt: Không đỡ mất `100 điểm` (ngã sàn ngay), Đỡ mất `20 điểm`.
        *   Nếu `target->stability <= 0`: đặt trạng thái nằm sàn `COMBAT_ANIM_DEAD`, ghi nhận `knockdownUntilMs = nowMs + 600` và `invincibleUntilMs = nowMs + 600`.
        *   Nếu chưa ngã sàn: Đưa target vào trạng thái đơ trúng đòn/block tương ứng và áp dụng lực đẩy lùi `knockbackX` (giảm 50% khi đỡ).
    *   `CombatRules_UpdateKnockdown`:
        *   **Hồi phục điểm kiên định:** Nếu nhân vật không ngã sàn và không bị trúng đòn trong vòng `1500 ms`, hồi phục đầy `stability = 100`.
        *   **Hồi phục nằm sàn:** Nếu hết thời gian nằm sàn (`nowMs >= actor->knockdownUntilMs`), chuyển nhân vật về `COMBAT_ANIM_IDLE` và đặt `stability = 100`.

---

### Bước 3: Tích hợp logic vào bộ điều khiển nhân vật (`combat_actor.c`)
1.  **Chỉnh sửa file:** [combat_actor.c](file:///d:/HUST/2025_2/Nh%C3%BAng/PROJECT_12_7/bleach_vs_naruto_STM32F429ZI/Core/Src/combat_actor.c)
    *   Thêm `#include "combat_rules.h"` lên đầu file.
    *   Trong hàm `CombatActor_Update`:
        *   Gọi `CombatRules_UpdateKnockdown(actor, nowMs)` ở ngay đầu hàm (dưới đoạn check `hp == 0`).
        *   Tìm nhánh xử lý nút bấm `COMBAT_INPUT_SKILL`, sửa điều kiện:
            ```c
            else if (((inputFlags & COMBAT_INPUT_SKILL) != 0U) && (CombatRules_CanUseSkill(actor) != 0U))
            {
              CombatRules_ConsumeSkillMana(actor);
              CombatActor_SetState(actor, COMBAT_ANIM_SKILL, nowMs);
            }
            ```
    *   Trong hàm `CombatActor_IsActionLocked`:
        *   Khóa thêm hành động khi nhân vật đang nằm sàn:
            ```c
            return ((nowMs < actor->stunUntilMs) ||
                    (nowMs < actor->knockdownUntilMs) ||
                    (actor->state == COMBAT_ANIM_ATTACK) ||
                    // ...
            ```

---

### Bước 4: Tích hợp luật va chạm mới vào Battle Demo (`battle_demo.c`)
1.  **Chỉnh sửa file:** [battle_demo.c](file:///d:/HUST/2025_2/Nh%C3%BAng/PROJECT_12_7/bleach_vs_naruto_STM32F429ZI/Core/Src/battle_demo.c)
    *   Thêm `#include "combat_rules.h"` lên đầu file.
    *   Trong hàm `Battle_ResolveHit`:
        Thay lời gọi hàm cũ bằng hàm luật chơi mới:
        ```c
        CombatRules_ProcessHit(attacker, target, hitbox, nowMs);
        ```

---

### Bước 5: Lập trình vẽ đồ họa thanh Mana 50x2 lên HUD (`battle_demo.c`)
1.  **Chỉnh sửa file:** [battle_demo.c](file:///d:/HUST/2025_2/Nh%C3%BAng/PROJECT_12_7/bleach_vs_naruto_STM32F429ZI/Core/Src/battle_demo.c)
    *   Mở rộng chiều cao khung đen HUD từ `35U` lên `42U` tại hàm `Battle_DrawHud`:
        `ILI9341_DrawFilledRectangleCoord(6U, 6U, 314U, 42U, RGB565_BLACK);`
    *   Tạo hàm mới `Battle_DrawManaBar(uint16_t x, uint16_t y, uint16_t mana, uint16_t color)`:
        *   Vẽ viền khung ngoài tại tọa độ `x, y` rộng `102`, cao `4`.
        *   Tô màu nền thanh mana rỗng và tô màu lấp đầy dựa trên giá trị `mana` hiện tại.
        *   Vẽ một vạch đen dọc 1-pixel ở vị trí chính giữa thanh Mana (tọa độ `x + 51`) để phân tách thành 2 vạch chiêu thức.
    *   Trong hàm `BattleDemo_Update`:
        *   Cập nhật vẽ lại thanh Mana của P1 và CPU khi giá trị thay đổi:
            ```c
            if (s_player.mana != s_lastPlayerMana)
            {
              Battle_DrawManaBar(12U, 32U, s_player.mana, RGB565_ACCENT_CYAN);
              s_lastPlayerMana = s_player.mana;
            }
            if (s_cpu.mana != s_lastCpuMana)
            {
              Battle_DrawManaBar(202U, 32U, s_cpu.mana, RGB565_ACCENT_CYAN);
              s_lastCpuMana = s_cpu.mana;
            }
            ```

---

## 3. Kịch bản kiểm thử (Verification Plan)
Tiến hành kiểm tra tuần tự:
1.  **Mana ban đầu:** Khởi đầu trận đấu, nhấn Skill -> nhân vật không ra chiêu (do mana = 0).
2.  **Tích lũy Mana:** Đánh thường trúng đối phương xem vạch mana có tăng dần (+10 mỗi đòn). Thử đánh khi đối phương đang đỡ đòn xem vạch mana chỉ nhích lên rất ít (+1).
3.  **Tung chiêu đặc biệt:** Khi mana đầy quá nửa (>=50), bấm Skill xem có ra chiêu và vạch mana có bị sụt giảm một nửa không.
4.  **Kiểm tra điểm giáp (Stability) ngầm:**
    *   Đánh thường liên tiếp 5 đòn khi CPU không đỡ -> CPU phải ngã sàn ở đòn thứ 5.
    *   Đánh thường khi CPU đang đỡ đòn -> CPU không được phép bị ngã sàn dù đánh bao nhiêu đòn thường.
    *   Tung chiêu đặc biệt của Player khi CPU đang đỡ đòn -> CPU phải bị mất 20 điểm giáp. Đỡ liên tục 5 chiêu đặc biệt -> CPU bị ngã sàn.
5.  **Kiểm phục hồi giáp:** Đánh CPU 2 đòn thường (stability còn 60). Dừng lại di chuyển giữ khoảng cách trong 1.5 giây. Đánh tiếp xem CPU có bị ngã sàn sau 3 đòn tiếp theo không (nếu phục hồi thành công thì phải mất đủ 5 đòn nữa).
