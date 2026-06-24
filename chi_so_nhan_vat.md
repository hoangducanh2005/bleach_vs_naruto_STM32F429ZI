# Đặc tả Chỉ số Nhân vật & Cơ chế Chiến đấu (Bleach vs Naruto STM32F429)

Tài liệu này định nghĩa chi tiết các chỉ số cơ bản, sát thương, cơ chế hồi năng lượng, đỡ đòn (block), và các hệ số điều chỉnh theo độ khó dành cho game đối kháng trên kit STM32F429. Các thông số được cân bằng để phù hợp với nhịp độ game 30 FPS và cấu trúc RAM/Flash của vi điều khiển.

---

## 1. Chỉ số Cơ bản của Nhân vật (General Stats)

| Chỉ số | Giá trị mặc định | Giải thích |
| :--- | :---: | :--- |
| **HP (Máu)** | `100` | Lượng máu tối đa của nhân vật. Khi HP về `0`, nhân vật chuyển sang trạng thái `DEAD`. |
| **Năng lượng (Mana/Energy)** | `90` | Dùng để thi triển kỹ năng đặc biệt (`SKILL`). Khởi đầu trận đấu là `0`. |
| **Tốc độ di chuyển ngang** | `4 pixel / frame` | Tốc độ chạy trái/phải trên mặt đất. |
| **Lực nhảy (Vận tốc Y ban đầu)** | `-12 pixel / frame`| Lực đẩy khi nhấn nhảy. |
| **Trọng lực (Gia tốc Y)** | `1 pixel / frame²` | Gia tốc rơi tự do kéo nhân vật về mặt đất (`GROUND_Y = 205`). |
| **Thời gian đơ khi trúng đòn (Hit Stun)**| `300 ms` (10 frames) | Thời gian nhân vật bị khựng khi trúng đòn, không thể nhận điều khiển. |
| **Thời gian bất tử (I-Frames)** | `500 ms` (15 frames) | Bắt đầu khi bị trúng đòn. Giúp tránh việc bị dính combo liên tục không thể thoát. |

---

## 2. Hệ thống Sát thương & Năng lượng (Damage & Mana)

### 2.1. Đòn đánh thường (Light Attack)
*   **Sát thương cơ bản**: `10 HP`
*   **Thời gian ra chiêu (Startup + Active)**: `200 ms` (khoảng 6 frames)
*   **Thời gian hồi chiêu (Cooldown)**: `350 ms`
*   **Cơ chế hồi Mana**:
    *   Đánh trúng đối thủ (không đỡ): **`+15 Mana`**
    *   Đánh trúng đối thủ đang đỡ (block): **`+5 Mana`**
    *   Đánh trượt (không va chạm): **`0 Mana`**

### 2.2. Chiêu đặc biệt (Special Skill)
Do giới hạn dung lượng bộ nhớ Flash/RAM trên kit STM32F429, mỗi nhân vật trong mã nguồn hiện tại chỉ được định nghĩa một kỹ năng đặc biệt duy nhất (`SKILL` state) ngoài đòn đánh thường (`ATTACK_LIGHT`). 

Chiêu thức này sẽ tiêu hao một phần Mana, cho phép người chơi sử dụng nhiều lần khi thanh Mana đầy (tương tự cơ chế kỹ năng tốn 1 thanh năng lượng trong game gốc):
*   **Mana tiêu hao**: Tiêu hao **`30 Mana`** cho mỗi lần kích hoạt (cho phép tung tối đa 3 chiêu liên tiếp khi đầy thanh 90 Mana).
*   **Cơ chế hồi Mana thụ động**: **`0 Mana / giây`** (Mana không tự động tăng theo thời gian, bắt buộc phải tích lũy từ đòn đánh thường trúng).

| Nhân vật | Tên kỹ năng | Mana tiêu hao | Sát thương | Đặc điểm chiêu thức |
| :--- | :--- | :---: | :---: | :--- |
| **Naruto** | *Rasengan (Loa Toàn Hoàn)* | `30` | `24 HP` | Tầm đánh cận chiến trung bình, hitbox rộng, tạo độ đẩy lùi (knockback) cực mạnh. |
| **Ichigo** | *Getsuga Tensho (Nguyệt Nha Thiên Xung)*| `30` | `20 HP` | Bắn ra luồng kiếm khí (projectile) bay ngang màn hình. Sát thương đa mục tiêu trên đường bay. |
| **Sasuke** | *Chidori (Thiên Điểu)* | `30` | `22 HP` | Lướt nhanh về phía trước một khoảng `60 pixel`. Có khả năng xuyên phá block (hoặc gây sát thương bào mòn cao). |

---

## 3. Cơ chế Đỡ đòn (Block Mechanics)

Nhân vật chuyển sang trạng thái `BLOCK` khi người chơi giữ nút đỡ đòn. Khi đang đỡ:
*   **Giảm sát thương**: Giảm **`80%`** sát thương nhận vào.
    *   Trúng đòn thường khi đang đỡ: Chỉ nhận `2 HP` sát thương (thay vì `10 HP`).
    *   Trúng chiêu đặc biệt khi đang đỡ: Chỉ nhận `4..5 HP` sát thương (thay vì `20..24 HP`).
*   **Đẩy lùi (Knockback)**: Lực đẩy lùi giảm đi `50%`.
*   **Hồi Mana của đối thủ**: Đối thủ chỉ được cộng `+5 Mana` khi đánh trúng tấm khiên đỡ (thay vì `+15 Mana`).

---

## 4. Hệ số điều chỉnh theo Độ khó (Difficulty Modifiers)

Khi chọn độ khó ở menu chính, các chỉ số của máy (CPU/AI) sẽ thay đổi để tạo thử thách cho người chơi:

| Độ khó | Sát thương CPU | Tốc độ phản ứng CPU | Khả năng tự động Đỡ đòn (Block) |
| :--- | :---: | :---: | :---: |
| **EASY** | `70%` | `800 ms` | `0%` (CPU không biết đỡ) |
| **NORMAL**| `100%` | `500 ms` | `20%` cơ hội đỡ khi người chơi áp sát |
| **HARD** | `120%` | `250 ms` | `45%` cơ hội đỡ; biết lùi lại khi máu thấp |

---

## 5. Cấu trúc Khai báo trong Mã nguồn C (`struct`)

Để hiện thực hóa các chỉ số này vào code, chúng ta sử dụng cấu trúc struct như sau trong file `player.h` hoặc `game.h`:

```c
// Định nghĩa các hằng số chỉ số
#define MAX_HP              100
#define MAX_MANA            90
#define MANA_REGEN_PASSIVE  0       // Mana không tự động tăng
#define INVINCIBLE_TIME_MS  500     // 500 ms bất tử sau khi trúng đòn
#define HIT_STUN_TIME_MS    300     // 300 ms đơ người

#define DAMAGE_LIGHT_ATTACK 10
#define MANA_GAIN_HIT       15      // Đánh trúng thường tăng 15 Mana
#define MANA_GAIN_BLOCKED   5       // Bị đỡ tăng 5 Mana
#define BLOCK_DAMAGE_REDUCE 80      // Giảm 80% sát thương (chỉ nhận 20%)

#define SKILL_MANA_COST     30      // Kỹ năng tốn 30 Mana (tiêu hao 1/3 thanh)

// Struct lưu trạng thái động của nhân vật
typedef struct {
  int16_t x;                // Vị trí X (tính theo pixel)
  int16_t y;                // Vị trí Y (tính theo pixel)
  int8_t vx;                // Vận tốc X
  int8_t vy;                // Vận tốc Y
  uint8_t facing;           // Hướng nhìn: 0 = Phải, 1 = Trái
  
  int16_t hp;               // Máu hiện tại (0 -> 100)
  int16_t mana;             // Năng lượng hiện tại (0 -> 100)
  
  uint8_t state;            // Trạng thái (IDLE, RUN, ATTACK, BLOCK, HIT, DEAD...)
  uint32_t stateTimer;      // Bộ đếm thời gian cho trạng thái hiện tại (ms)
  uint32_t invincibleTimer; // Bộ đếm thời gian bất tử còn lại (ms)
  uint32_t cooldownTimer;   // Bộ đếm thời gian hồi đòn đánh thường (ms)
} CharacterStats;

---

## 6. Vật lý Đẩy lùi (Knockback Physics)

Để các đòn đánh có cảm giác "nặng đô" và chân thực, khi nhân vật trúng đòn sẽ bị đẩy lùi về phía sau (ngược hướng nhìn của đối thủ):
*   **Đòn đánh thường (`ATTACK_LIGHT`)**: 
    *   Khoảng cách đẩy lùi: `12 pixel` (giảm còn `6 pixel` nếu đối thủ đang đỡ).
    *   Tốc độ đẩy lùi giảm dần trong suốt thời gian bị đơ (`Hit Stun`).
*   **Chiêu đặc biệt (`SKILL`)**:
    *   Khoảng cách đẩy lùi: `35 pixel` (giảm còn `15 pixel` nếu đối thủ đang đỡ).
    *   Tạo khoảng cách an toàn cho cả hai bên sau khi tung chiêu lớn.

---

## 7. Cơ chế Va chạm (Collision & Hitboxes)

Game đối kháng trên STM32 sử dụng thuật toán **AABB (Axis-Aligned Bounding Box)** để kiểm tra va chạm giữa hai hình chữ nhật. Mỗi nhân vật trong từng khung hình hoạt ảnh sẽ có hai vùng hộp:

1.  **Bodybox (Hộp nhận sát thương)**: Vùng bao quanh cơ thể nhân vật để kiểm tra xem có bị trúng đòn không.
    *   *Kích thước trung bình*: Rộng `24 pixel`, Cao `42 pixel`.
2.  **Hitbox (Hộp ra đòn)**: Vùng gây sát thương chỉ xuất hiện tại các frame chủ động của đòn đánh thường hoặc kỹ năng.
    *   *Đòn thường*: Xuất hiện phía trước nhân vật với khoảng cách `12 pixel`, rộng `16 pixel`.
    *   *Getsuga Tensho (Ichigo)*: Đi theo luồng kiếm khí (projectile) di chuyển độc lập.

### 7.1. Thuật toán kiểm tra va chạm AABB (mã C mẫu)
```c
typedef struct {
  int16_t x;
  int16_t y;
  uint16_t w;
  uint16_t h;
} Rect;

uint8_t CheckCollision(Rect r1, Rect r2) {
  return (r1.x < r2.x + r2.w &&
          r1.x + r1.w > r2.x &&
          r1.y < r2.y + r2.h &&
          r1.y + r1.h > r2.y);
}
```

### 7.2. Cơ chế Đỡ đòn từ phía sau (Cross-up / Backstab)
*   **Nguyên tắc**: Đỡ đòn (`BLOCK`) chỉ có tác dụng nếu đối thủ đứng ở phía trước mặt nhân vật.
*   **Bỏ qua Block**: Nếu nhân vật nhảy qua đầu và tấn công từ phía sau lưng đối thủ, trạng thái `BLOCK` sẽ bị bỏ qua và đối thủ vẫn nhận 100% sát thương đòn đánh. Điều này tạo thêm chiều sâu chiến thuật (Cross-up) cho game.

---

## 8. Chặn Biên Màn hình (Screen Boundaries)

Màn hình ILI9341 có chiều rộng `320 pixel`. Để tránh nhân vật đi ra ngoài màn hình:
*   Tọa độ `x` của nhân vật phải được giới hạn trong khoảng từ **`5`** đến **`320 - width`** (ví dụ `320 - 24 = 296`).
*   Khi bị đẩy lùi sát biên màn hình, nhân vật trúng đòn sẽ không thể lùi thêm, thay vào đó đối thủ ra đòn sẽ bị đẩy nhẹ ngược lại (vật lý phản lực biên) để giữ khoảng cách trực quan.

---

## 9. Cơ chế Knockdown (Nằm sàn / Ngã sàn) & Chống Combo Vô hạn

Để tái hiện đúng cơ chế của game gốc và tránh việc bị đối thủ combo liên tục đến chết:

### 9.1. Trạng thái Nằm sàn tái sử dụng hoạt ảnh `DEAD`
*   **Vấn đề**: Tài nguyên sprite hiện tại không có hoạt ảnh nằm sàn riêng biệt (chỉ có `HIT` đứng đơ và `DEAD` ngã sàn).
*   **Giải pháp**: Khi nhân vật bị đánh ngã (Knockdown), game sẽ kích hoạt hoạt ảnh **`DEAD`** nhưng **không xử lý thua cuộc**.
    *   Nhân vật sẽ chơi hoạt ảnh ngã xuống và nằm im trên sàn (các frame cuối của `DEAD`).
    *   Trong suốt thời gian nằm sàn này (ví dụ `600 ms`), nhân vật ở trạng thái **Bất tử hoàn toàn (Invincible)**, đối thủ không thể đánh trúng (đòn đánh sẽ đi xuyên qua).
    *   Sau khi hết thời gian nằm sàn, nhân vật sẽ chuyển thẳng về trạng thái **`IDLE`** (đứng dậy) và có thể tiếp tục chiến đấu.

### 9.2. Điểm Kiên định (Stability/Poise Point)
Để quyết định khi nào nhân vật bị ngã sàn:
*   Mỗi nhân vật có một chỉ số ẩn là **Điểm Kiên định (Max = 100)**.
*   Khi bị đánh trúng:
    *   Đòn đánh thường (`ATTACK_LIGHT`): Trừ `35` điểm kiên định. Nhân vật chỉ bị đơ nhẹ đứng yên (`HIT` state).
    *   Kỹ năng đặc biệt (`SKILL`): Trừ thẳng `100` điểm kiên định (gây ngã sàn ngay lập tức).
*   Khi Điểm Kiên định giảm về `0` (ví dụ dính 3 đòn đánh thường liên tiếp): Nhân vật sẽ **ngã sàn ngay lập tức** (Knockdown) và được bảo vệ bởi trạng thái bất tử.
*   Khi nhân vật đứng dậy (`IDLE`), Điểm Kiên định tự động reset về `100`.


```
