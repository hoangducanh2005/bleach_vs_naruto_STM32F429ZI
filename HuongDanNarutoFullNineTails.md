# Tài Liệu Kỹ Thuật Nhân Vật Naruto Full Nine Tails

Tài liệu này mô tả cấu trúc dữ liệu hoạt ảnh, các trạng thái (states), cơ chế đạn bay (projectile) của nhân vật **Naruto Full Nine Tails** trong dự án game đối kháng chạy trên vi điều khiển STM32F429ZI.

---

## 1. Các File Thành Phần

*   **Dữ liệu hoạt ảnh và cấu hình:**
    *   **[naruto_full_nine_tails_moveset.h](file:///Core/Inc/naruto_full_nine_tails_moveset.h)**: Định nghĩa enum trạng thái, cấu trúc frame/animation và khai báo mảng hoạt ảnh.
    *   **[naruto_full_nine_tails_moveset.c](file:///Core/Src/naruto_full_nine_tails_moveset.c)**: Chứa mảng dữ liệu pixel (RGB565) và khởi tạo mảng hoạt ảnh.
    *   **[naruto_full_nine_tails_moveset.json](file:///assets/raw/naruto_full_nine_tails_frames/naruto_full_nine_tails_moveset.json)**: File cấu hình frame (đường dẫn strip, duration, loop, projectile).

*   **Chương trình kiểm thử & Demo:**
    *   **[naruto_full_nine_tails_animation_demo.c](file:///Core/Src/naruto_full_nine_tails_animation_demo.c)**: Demo hiển thị tuần tự các hoạt ảnh trên màn hình LCD ILI9341.

---

## 2. Cấu Trúc Dữ Liệu Hoạt Ảnh

Được định nghĩa trong **[naruto_full_nine_tails_moveset.h](file:///Core/Inc/naruto_full_nine_tails_moveset.h)**:

### NarutoFullNineTailsMoveFrame (Khung hình đơn)
```c
typedef struct
{
  const uint16_t *pixels; // Mảng màu RGB565 của khung hình
  uint16_t width;         // Chiều rộng khung hình (px)
  uint16_t height;        // Chiều cao khung hình (px)
  int16_t pivotX;         // Tọa độ X điểm neo (tâm ngang)
  int16_t pivotY;         // Tọa độ Y điểm neo (chân nhân vật)
  uint16_t durationMs;    // Thời gian hiển thị khung hình (ms)
} NarutoFullNineTailsMoveFrame;
```

### NarutoFullNineTailsMoveAnimation (Hoạt ảnh trạng thái)
```c
typedef struct
{
  const NarutoFullNineTailsMoveFrame *frames;  // Mảng các khung hình nhân vật
  uint8_t frameCount;                          // Số lượng khung hình
  uint8_t loop;                                // 1: Lặp lại, 0: Chạy một lần
  uint8_t holdLast;                            // 1: Giữ frame cuối khi kết thúc hoạt ảnh

  // Hiệu ứng tích tụ năng lượng (Caster Effect)
  const uint16_t * const *casterEffectFrames;  // Mảng ảnh hiệu ứng
  uint8_t casterEffectFrameCount;
  uint8_t casterEffectStartFrameIdx;           // Frame bắt đầu xuất hiện hiệu ứng
  uint8_t casterEffectLoop;
  uint16_t casterEffectWidth;
  uint16_t casterEffectHeight;

  // Quả cầu năng lượng bay đi (Projectile)
  const uint16_t * const *projectileFrames;    // Mảng ảnh quả cầu bay đi
  uint8_t projectileFrameCount;
  uint8_t projectileLoop;
  uint16_t projectileWidth;
  uint16_t projectileHeight;
} NarutoFullNineTailsMoveAnimation;
```

---

## 3. Danh Sách Trạng Thái Hoạt Ảnh (States)

Ánh xạ trạng thái tương ứng với enum **[NarutoFullNineTailsMoveState](file:///Core/Inc/naruto_full_nine_tails_moveset.h#L19-L33)**:

| Enum State | Số Frame | Kích thước (W x H) | Neo Pivot (X, Y) | Duration | Loop | Hold | Thư mục strip gốc |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :--- |
| `NARUTO_FULL_NINE_TAILS_MOVE_IDLE` | 2 | 82 x 64 | (41, 64) | 140 ms | Có | Không | `naruto_full_nine_tails_1` |
| `NARUTO_FULL_NINE_TAILS_MOVE_RUN` | 4 | 98 x 64 | (49, 64) | 80 ms | Có | Không | `naruto_full_nine_tails_2` |
| `NARUTO_FULL_NINE_TAILS_MOVE_SLIDE` | 1 | 96 x 55 | (48, 55) | 90 ms | Không | Không | `naruto_full_nine_tails_3` |
| `NARUTO_FULL_NINE_TAILS_MOVE_JUMP` | 5 | 86 x 88 | (43, 88) | 90 ms | Không | Không | `naruto_full_nine_tails_4` |
| `NARUTO_FULL_NINE_TAILS_MOVE_BLOCK` | 1 | 88 x 88 | (44, 88) | 120 ms | Không | Có | `naruto_full_nine_tails_6` |
| `NARUTO_FULL_NINE_TAILS_MOVE_HIT` | 1 | 76 x 73 | (38, 73) | 90 ms | Không | Không | `naruto_full_nine_tails_7` |
| `NARUTO_FULL_NINE_TAILS_MOVE_ATTACK1` | 2 | 100 x 62 | (50, 62) | 70 ms | Không | Không | `naruto_full_nine_tails_9` |
| `NARUTO_FULL_NINE_TAILS_MOVE_ATTACK2` | 2 | 92 x 67 | (46, 67) | 70 ms | Không | Không | `naruto_full_nine_tails_14` |
| `NARUTO_FULL_NINE_TAILS_MOVE_ATTACK3` | 3 | 90 x 72 | (45, 72) | 80 ms | Không | Không | `naruto_full_nine_tails_10` |
| `NARUTO_FULL_NINE_TAILS_MOVE_SKILL1` | 3 | 182 x 129 | (41, 129) | 70 ms | Không | Không | `naruto_full_nine_tails_17` / `_18` |
| `NARUTO_FULL_NINE_TAILS_MOVE_DEAD` | 5 | 94 x 103 | (47, 103) | 90 ms | Không | Có | `naruto_full_nine_tails_8` |

> [!NOTE]
> Khung hình trạng thái `SKILL1` có điểm neo X (`pivotX = 41`) được đặt bằng với `IDLE` thay vì `width // 2` để đảm bảo nhân vật giữ nguyên vị trí vẽ khi kích hoạt chiêu thức.

---

## 4. Cơ Chế Đạn Bay (Projectile)

Logic xử lý đạn bay nằm trong **[battle_demo.c](file:///Core/Src/battle_demo.c)**:

*   **Khởi tạo (`Battle_TrySpawnNinetailsBomb`):**
    *   Kích hoạt khi nhân vật ở trạng thái `COMBAT_ANIM_SKILL`, tại frame index 0.
    *   Tài nguyên ảnh lấy từ `projectileFrames` trong hoạt ảnh `SKILL1` (kích thước `112 x 67` px).
    *   Tọa độ Y bắt đầu: `BATTLE_GROUND_Y - 67 = 138`.
    *   Tọa độ X bắt đầu: `player_x - 18 - 112` (hướng quay trái) hoặc `player_x + 18` (hướng quay phải).

*   **Cập nhật (`Battle_UpdateNinetailsBomb`):**
    *   Tịnh tiến X theo vận tốc `BATTLE_NINETAILS_BOMB_SPEED`.
    *   Thay đổi frame hoạt ảnh dựa trên thời gian và cấu hình lặp.
    *   Hủy đạn khi tọa độ vượt ra ngoài phạm vi màn hình LCD (`X < -112` hoặc `X > 320`).

---

## 5. Chương Trình Demo Hoạt Ảnh

Logic hoạt động của **[naruto_full_nine_tails_animation_demo.c](file:///Core/Src/naruto_full_nine_tails_animation_demo.c)**:

*   Quét tuần tự các trạng thái định nghĩa trong `s_sequence` để hiển thị trên LCD.
*   Vẽ nền tĩnh (bầu trời, mặt trời, mây, núi, tường dojo, sàn đấu).