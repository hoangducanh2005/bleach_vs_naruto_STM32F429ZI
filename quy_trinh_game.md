# Quy trình thực hiện project Game đối kháng Naruto vs Bleach trên STM32F429I

## 1. Mục tiêu project

Xây dựng game đối kháng 2D chạy trên kit STM32F429I/STM32F429ZIT6, hiển thị bằng LCD, điều khiển bằng joystick/nút nhấn và phát âm thanh đơn giản bằng buzzer.

Game có hai nhân vật cố định lấy cảm hứng từ phong cách đối kháng Naruto vs Bleach:

- Nhân vật 1: Naruto.
- Nhân vật 2: Ichigo hoặc một nhân vật Bleach cố định.
- Chế độ chơi: người chơi đấu với máy.
- Có menu chọn nhân vật hoặc chọn mức độ khó.
- Có giới hạn thời gian trận đấu.
- Có thanh HP, thanh năng lượng, trạng thái nhân vật, va chạm đòn đánh, AI đơn giản và hiệu ứng âm thanh.

Lưu ý: với đồ án học tập, nên tự vẽ sprite đơn giản hoặc dùng hình thay thế tự tạo. Không nên lấy trực tiếp tài nguyên hình ảnh/âm thanh từ các game web như Y8 nếu chưa có quyền sử dụng.

## 2. Phạm vi chức năng

### 2.1. Chức năng bắt buộc

- Hiển thị màn hình menu.
- Chọn mức độ khó: Easy, Normal, Hard.
- Hiển thị sân đấu 2D.
- Hai nhân vật cố định xuất hiện trên màn hình.
- Người chơi điều khiển nhân vật bằng joystick/nút:
  - Di chuyển trái.
  - Di chuyển phải.
  - Nhảy.
  - Tấn công thường.
  - Dùng kỹ năng.
- Nhân vật có các trạng thái:
  - Đứng.
  - Chạy.
  - Nhảy.
  - Tấn công.
  - Trúng đòn.
  - Chết.
- Quản lý HP và năng lượng.
- Phát hiện vùng va chạm khi tấn công.
- AI máy hoạt động theo luật đơn giản.
- Có bộ đếm thời gian trận đấu.
- Có âm thanh khi:
  - Ra đòn.
  - Trúng đòn.
  - Hết trận/thắng/thua.

### 2.2. Chức năng mở rộng nếu còn thời gian

- Thêm màn hình hướng dẫn điều khiển.
- Thêm hiệu ứng nhấp nháy khi trúng đòn.
- Thêm hoạt ảnh đơn giản cho từng trạng thái.
- Thêm hiệu ứng kỹ năng bay xa.
- Thêm pause game.

## 3. Phần cứng sử dụng

| Thành phần | Vai trò |
|---|---|
| STM32F429ZIT6 / STM32F429I Discovery | Vi điều khiển chính |
| LCD TFT | Hiển thị menu, sân đấu, nhân vật, HP, năng lượng |
| Joystick hoặc nút nhấn | Điều khiển trái/phải/nhảy/tấn công/kỹ năng |
| Buzzer | Phát âm thanh hiệu ứng |
| Timer | Tạo tick game, PWM cho buzzer, quản lý thời gian |
| GPIO | Đọc nút, điều khiển buzzer nếu dùng dạng on/off |

## 4. Công cụ phát triển

- STM32CubeIDE.
- STM32CubeMX để cấu hình clock, GPIO, timer, LCD.
- Ngôn ngữ C.
- Thư viện HAL.
- Thư viện LCD đi kèm board hoặc thư viện BSP nếu dùng STM32F429I Discovery.

## 5. Cấu hình ngoại vi đề xuất

### 5.1. LCD

- Dùng LCD làm thiết bị hiển thị chính.
- Độ phân giải thường gặp trên STM32F429I Discovery: 240 x 320.
- Nên xoay màn hình ngang nếu thư viện hỗ trợ để game có không gian di chuyển rộng hơn.
- Dùng double buffering nếu có đủ RAM/SDRAM; nếu không, chỉ vẽ lại các vùng thay đổi.

### 5.2. Nút nhấn/joystick

Gợi ý ánh xạ điều khiển:

| Input | Hành động |
|---|---|
| Left | Di chuyển trái |
| Right | Di chuyển phải |
| Up | Nhảy |
| Button A | Tấn công thường |
| Button B | Kỹ năng |
| Button Start | Chọn menu / bắt đầu trận |

Nếu số nút hạn chế, có thể dùng:

- Giữ joystick lên để nhảy.
- Nhấn nút 1 để đánh thường.
- Nhấn nút 2 hoặc giữ nút 1 lâu để dùng kỹ năng.

### 5.3. Buzzer

- Dùng PWM từ timer để tạo âm.
- Mỗi loại sự kiện phát một tần số/thời lượng khác nhau:
  - Đánh thường: âm ngắn, tần số trung bình.
  - Trúng đòn: âm ngắn, tần số thấp.
  - Kỹ năng: âm dài hơn, tần số cao.
  - Kết thúc trận: chuỗi âm 2-3 nốt.

### 5.4. Timer game

- Dùng một timer hoặc SysTick để tạo chu kỳ cập nhật game.
- Tần số đề xuất:
  - Logic game: 30 FPS hoặc 60 FPS.
  - Nếu LCD vẽ chậm, ưu tiên 30 FPS.
- Mỗi frame xử lý theo thứ tự:
  1. Đọc input.
  2. Cập nhật AI.
  3. Cập nhật trạng thái nhân vật.
  4. Kiểm tra va chạm.
  5. Cập nhật HP/năng lượng/thời gian.
  6. Vẽ màn hình.

## 6. Kiến trúc phần mềm

### 6.1. Cấu trúc file đề xuất

```text
Core/
  Inc/
    game.h
    player.h
    ai.h
    input.h
    render.h
    sound.h
    menu.h
    collision.h
  Src/
    game.c
    player.c
    ai.c
    input.c
    render.c
    sound.c
    menu.c
    collision.c
```

### 6.2. Vai trò từng module

| Module | Vai trò |
|---|---|
| game.c/h | Vòng lặp game, quản lý trạng thái màn hình |
| player.c/h | Dữ liệu nhân vật, state machine, di chuyển, đánh |
| ai.c/h | Điều khiển nhân vật máy |
| input.c/h | Đọc joystick/nút và chống dội phím |
| render.c/h | Vẽ sân đấu, nhân vật, thanh HP, năng lượng |
| sound.c/h | Phát âm thanh bằng buzzer |
| menu.c/h | Menu chọn độ khó/nhân vật |
| collision.c/h | Kiểm tra vùng va chạm đòn đánh |

## 7. Thiết kế dữ liệu chính

### 7.1. Enum trạng thái nhân vật

```c
typedef enum
{
    STATE_IDLE,
    STATE_RUN,
    STATE_JUMP,
    STATE_ATTACK,
    STATE_SKILL,
    STATE_HIT,
    STATE_DEAD
} CharacterState;
```

### 7.2. Struct nhân vật

```c
typedef struct
{
    int x;
    int y;
    int vx;
    int vy;
    int width;
    int height;

    int hp;
    int maxHp;
    int energy;
    int maxEnergy;

    int facing;          // -1: nhìn trái, 1: nhìn phải
    CharacterState state;

    int stateTime;
    int attackCooldown;
    int skillCooldown;
    int invincibleTime;
} Character;
```

### 7.3. Struct vùng va chạm

```c
typedef struct
{
    int x;
    int y;
    int width;
    int height;
    int damage;
    int active;
} HitBox;
```

## 8. State machine nhân vật

### 8.1. Luồng trạng thái

```text
IDLE
  -> RUN khi nhấn trái/phải
  -> JUMP khi nhấn nhảy
  -> ATTACK khi nhấn đánh thường
  -> SKILL khi nhấn kỹ năng và đủ năng lượng
  -> HIT khi bị trúng đòn
  -> DEAD khi HP <= 0

RUN
  -> IDLE khi thả trái/phải
  -> JUMP khi nhấn nhảy
  -> ATTACK khi nhấn đánh thường
  -> SKILL khi nhấn kỹ năng và đủ năng lượng
  -> HIT khi bị trúng đòn
  -> DEAD khi HP <= 0

JUMP
  -> IDLE hoặc RUN khi chạm đất
  -> ATTACK nếu cho phép đánh trên không
  -> HIT khi bị trúng đòn
  -> DEAD khi HP <= 0

ATTACK/SKILL
  -> IDLE khi hết animation/thời gian ra đòn
  -> HIT nếu bị đánh trúng và không khóa trạng thái
  -> DEAD khi HP <= 0

HIT
  -> IDLE khi hết thời gian choáng
  -> DEAD khi HP <= 0
```

### 8.2. Thời gian trạng thái đề xuất

| Trạng thái | Thời gian |
|---|---|
| ATTACK | 200-300 ms |
| SKILL | 400-600 ms |
| HIT | 300 ms |
| Nhảy | Theo vận tốc và trọng lực |
| DEAD | Giữ nguyên đến hết trận |

## 9. Cơ chế di chuyển và vật lý đơn giản

### 9.1. Tọa độ

- Trục X: trái sang phải.
- Trục Y: trên xuống dưới.
- Mặt đất có tọa độ cố định, ví dụ `GROUND_Y = 200`.

### 9.2. Di chuyển ngang

- Khi nhấn trái: `vx = -PLAYER_SPEED`.
- Khi nhấn phải: `vx = PLAYER_SPEED`.
- Khi không nhấn: `vx = 0`.
- Giới hạn nhân vật không đi ra khỏi màn hình.

### 9.3. Nhảy

- Khi đang đứng trên đất và nhấn nhảy:
  - `vy = -JUMP_FORCE`.
- Mỗi frame:
  - `vy += GRAVITY`.
  - `y += vy`.
- Nếu `y >= GROUND_Y`:
  - `y = GROUND_Y`.
  - `vy = 0`.
  - chuyển về `IDLE` hoặc `RUN`.

## 10. Tấn công và va chạm

### 10.1. Đòn đánh thường

- Điều kiện:
  - Không trong trạng thái DEAD/HIT.
  - Hết cooldown.
- Tạo hitbox phía trước nhân vật.
- Tầm đánh ngắn.
- Sát thương đề xuất: 8-12 HP.
- Tăng năng lượng khi đánh trúng.

### 10.2. Kỹ năng

- Điều kiện:
  - Năng lượng đủ, ví dụ >= 30.
  - Hết cooldown.
- Tạo hitbox lớn hơn hoặc projectile bay thẳng.
- Sát thương đề xuất: 18-25 HP.
- Trừ năng lượng khi dùng.

### 10.3. Kiểm tra va chạm

Dùng kiểm tra hình chữ nhật giao nhau:

```c
int Collision_CheckRect(Rect a, Rect b)
{
    return (a.x < b.x + b.width &&
            a.x + a.width > b.x &&
            a.y < b.y + b.height &&
            a.y + a.height > b.y);
}
```

Khi hitbox của người tấn công chạm body box của đối thủ:

- Trừ HP đối thủ.
- Chuyển đối thủ sang trạng thái HIT.
- Đẩy lùi nhẹ đối thủ.
- Phát âm thanh trúng đòn.
- Tắt hitbox hoặc đánh dấu đã gây sát thương để không trừ HP nhiều lần trong cùng một đòn.

## 11. Quản lý HP, năng lượng và thời gian

### 11.1. HP

- Mỗi nhân vật có `maxHp = 100`.
- HP giảm khi trúng đòn.
- Nếu HP <= 0:
  - HP = 0.
  - Chuyển trạng thái DEAD.
  - Kết thúc trận.

### 11.2. Năng lượng

Nguồn tăng năng lượng:

- Tăng chậm theo thời gian.
- Tăng khi đánh trúng đối thủ.
- Có giới hạn `maxEnergy = 100`.

Dùng kỹ năng:

- Trừ 30-50 năng lượng.
- Nếu không đủ năng lượng thì không cho dùng kỹ năng.

### 11.3. Thời gian trận đấu

- Mỗi trận kéo dài 60 hoặc 90 giây.
- Hiển thị thời gian còn lại trên LCD.
- Khi hết giờ:
  - Nhân vật có HP cao hơn thắng.
  - Nếu HP bằng nhau thì hòa.

## 12. AI người máy

### 12.1. Mục tiêu AI

AI không cần phức tạp, chỉ cần tạo cảm giác đối thủ biết tiếp cận, giữ khoảng cách và tấn công.

### 12.2. Luật AI cơ bản

Tính khoảng cách:

```c
distance = abs(player.x - enemy.x);
```

Luật hành động:

- Nếu AI đang chết hoặc trúng đòn: không điều khiển.
- Nếu khoảng cách quá xa:
  - Chạy lại gần người chơi.
- Nếu khoảng cách vừa:
  - Có xác suất dùng kỹ năng nếu đủ năng lượng.
  - Nếu không dùng kỹ năng thì tiếp tục áp sát.
- Nếu khoảng cách gần:
  - Có xác suất đánh thường.
  - Có xác suất lùi lại để tránh spam.
- Nếu người chơi đang dùng kỹ năng:
  - AI có thể lùi hoặc nhảy né tùy độ khó.

### 12.3. Mức độ khó

| Mức | Đặc điểm |
|---|---|
| Easy | Phản ứng chậm, ít đánh, ít dùng kỹ năng |
| Normal | Phản ứng vừa, biết tiếp cận và đánh |
| Hard | Phản ứng nhanh, đánh thường xuyên, dùng kỹ năng hợp lý |

Thông số gợi ý:

| Thông số | Easy | Normal | Hard |
|---|---:|---:|---:|
| Thời gian ra quyết định | 800 ms | 500 ms | 250 ms |
| Xác suất đánh gần | 30% | 50% | 70% |
| Xác suất dùng kỹ năng | 10% | 25% | 40% |
| Khả năng né | Thấp | Vừa | Cao |

## 13. Menu game

### 13.1. Các màn hình

```text
MENU_MAIN
  -> Chọn Start
  -> Chọn Difficulty
  -> Chọn Character nếu muốn có chức năng chọn nhân vật

MENU_DIFFICULTY
  -> Easy
  -> Normal
  -> Hard

GAME_PLAYING
  -> GAME_OVER khi hết HP hoặc hết giờ

GAME_OVER
  -> Hiển thị Win/Lose/Draw
  -> Nhấn Start để quay lại menu
```

### 13.2. Menu chọn nhân vật

Vì đề tài ưu tiên hai nhân vật cố định, có thể làm đơn giản:

- Người chơi chọn Naruto hoặc Bleach.
- Máy tự động nhận nhân vật còn lại.

Nếu muốn giảm khối lượng, chỉ cần menu chọn mức độ khó và giữ Naruto là người chơi, Bleach là máy.

## 14. Render LCD

### 14.1. Thành phần cần vẽ

- Background sân đấu.
- Nhân vật 1.
- Nhân vật 2.
- Thanh HP nhân vật 1.
- Thanh HP nhân vật 2.
- Thanh năng lượng nhân vật 1.
- Thanh năng lượng nhân vật 2.
- Thời gian trận đấu.
- Hiệu ứng tấn công/kỹ năng.
- Màn hình game over.

### 14.2. Chiến lược vẽ

Nếu có framebuffer:

- Vẽ toàn bộ frame vào buffer.
- Đẩy buffer ra LCD.
- Giảm nhấp nháy.

Nếu không có framebuffer:

- Xóa vùng cũ của nhân vật.
- Vẽ lại background vùng đó.
- Vẽ nhân vật ở vị trí mới.
- Chỉ cập nhật thanh HP/năng lượng khi giá trị thay đổi.

### 14.3. Sprite đơn giản

Do bộ nhớ vi điều khiển hạn chế, có thể dùng:

- Hình chữ nhật màu cho nhân vật ở giai đoạn đầu.
- Bitmap nhỏ 16-bit RGB565.
- Sprite ít frame:
  - Idle: 1-2 frame.
  - Run: 2-4 frame.
  - Attack: 2 frame.
  - Hit: 1 frame.
  - Dead: 1 frame.

## 15. Âm thanh buzzer

### 15.1. API đề xuất

```c
void Sound_PlayAttack(void);
void Sound_PlayHit(void);
void Sound_PlaySkill(void);
void Sound_PlayGameOver(void);
void Sound_Update(void);
```

### 15.2. Nguyên tắc

- Không dùng `HAL_Delay()` dài trong lúc chơi vì sẽ làm đứng game.
- Âm thanh nên chạy theo timer hoặc biến đếm thời gian.
- `Sound_Update()` được gọi mỗi frame để tự tắt buzzer khi hết thời lượng.

## 16. Vòng lặp game tổng quát

```c
while (1)
{
    uint32_t now = HAL_GetTick();

    if (now - lastFrameTime >= FRAME_TIME_MS)
    {
        lastFrameTime = now;

        Input_Update();

        switch (gameScreen)
        {
            case SCREEN_MENU:
                Menu_Update();
                Menu_Render();
                break;

            case SCREEN_PLAY:
                Game_UpdateTimer();
                AI_Update();
                Player_Update(&player);
                Player_Update(&enemy);
                Collision_Update();
                Sound_Update();
                Render_Game();
                break;

            case SCREEN_GAME_OVER:
                GameOver_Update();
                GameOver_Render();
                Sound_Update();
                break;
        }
    }
}
```

## 17. Các bước thực hiện project

### Bước 1: Chuẩn bị project STM32CubeIDE

- Tạo project cho STM32F429ZIT6 hoặc mở project có sẵn.
- Cấu hình clock hệ thống.
- Cấu hình GPIO cho nút/joystick.
- Cấu hình LCD hoặc BSP LCD.
- Cấu hình timer cho buzzer PWM.
- Build thử project rỗng để đảm bảo không lỗi.

Kết quả cần đạt:

- Nạp được chương trình vào board.
- LCD hiển thị được màu hoặc chữ thử nghiệm.
- Đọc được trạng thái nút.
- Buzzer phát được âm thử nghiệm.

### Bước 2: Làm màn hình menu

- Tạo module `menu.c/h`.
- Vẽ menu chính.
- Cho phép di chuyển lựa chọn bằng joystick.
- Nhấn nút để chọn Start hoặc Difficulty.
- Lưu mức độ khó vào biến cấu hình.

Kết quả cần đạt:

- Có thể chọn mức độ khó.
- Có thể bắt đầu trận đấu từ menu.

### Bước 3: Tạo màn hình gameplay cơ bản

- Tạo module `game.c/h`.
- Vẽ background sân đấu.
- Vẽ hai nhân vật bằng hình chữ nhật hoặc sprite tạm.
- Vẽ HP, năng lượng và thời gian.

Kết quả cần đạt:

- Vào trận đấu thấy đủ hai nhân vật và giao diện cơ bản.

### Bước 4: Điều khiển nhân vật người chơi

- Tạo module `input.c/h`.
- Tạo module `player.c/h`.
- Cài đặt di chuyển trái/phải.
- Cài đặt nhảy và trọng lực.
- Giới hạn nhân vật trong màn hình.
- Chuyển trạng thái IDLE/RUN/JUMP.

Kết quả cần đạt:

- Người chơi di chuyển và nhảy mượt.
- Nhân vật không đi ra ngoài màn hình.

### Bước 5: Cài đặt tấn công thường

- Thêm trạng thái ATTACK.
- Thêm cooldown đánh thường.
- Tạo hitbox phía trước nhân vật.
- Kiểm tra va chạm với đối thủ.
- Trừ HP nếu đánh trúng.
- Phát âm thanh đánh và trúng đòn.

Kết quả cần đạt:

- Khi đứng gần và nhấn đánh, đối thủ mất HP.
- Khi đứng xa, đánh không trúng.

### Bước 6: Cài đặt kỹ năng

- Thêm trạng thái SKILL.
- Kiểm tra đủ năng lượng mới cho dùng.
- Tạo hitbox lớn hoặc projectile.
- Trừ năng lượng khi dùng.
- Gây sát thương cao hơn đánh thường.
- Thêm hiệu ứng vẽ đơn giản cho kỹ năng.

Kết quả cần đạt:

- Kỹ năng hoạt động khác đánh thường.
- Không thể dùng kỹ năng khi thiếu năng lượng.

### Bước 7: Cài đặt trạng thái trúng đòn và chết

- Khi bị đánh trúng, chuyển sang HIT.
- Trong HIT, nhân vật bị khựng trong thời gian ngắn.
- Khi HP <= 0, chuyển sang DEAD.
- Khi một nhân vật DEAD, chuyển sang màn hình GAME_OVER.

Kết quả cần đạt:

- Trận đấu kết thúc đúng khi một nhân vật hết HP.

### Bước 8: Cài đặt AI

- Tạo module `ai.c/h`.
- AI đọc vị trí người chơi và nhân vật máy.
- AI ra quyết định theo khoảng cách.
- Điều chỉnh thông số theo độ khó.

Kết quả cần đạt:

- Máy biết lại gần, đánh và thỉnh thoảng dùng kỹ năng.
- Mức Hard khó hơn Easy rõ ràng.

### Bước 9: Cài đặt thời gian trận đấu

- Tạo biến `matchTime`.
- Giảm thời gian theo giây.
- Vẽ thời gian còn lại lên LCD.
- Khi hết giờ, so sánh HP để xác định thắng/thua/hòa.

Kết quả cần đạt:

- Trận đấu tự kết thúc khi hết thời gian.

### Bước 10: Hoàn thiện đồ họa và âm thanh

- Thay hình chữ nhật bằng sprite đơn giản.
- Thêm màu riêng cho từng nhân vật.
- Thêm hiệu ứng khi trúng đòn.
- Thêm âm thanh cho attack, hit, skill, game over.
- Tối ưu vẽ để giảm nhấp nháy.

Kết quả cần đạt:

- Game nhìn rõ nhân vật, trạng thái và thông tin trận đấu.
- Âm thanh phản hồi đúng sự kiện.

### Bước 11: Kiểm thử và sửa lỗi

Kiểm thử các trường hợp:

- Nhấn nhiều nút cùng lúc.
- Đánh khi đang nhảy.
- Dùng kỹ năng khi không đủ năng lượng.
- Hai nhân vật sát mép màn hình.
- HP về 0 đúng lúc hết giờ.
- AI không bị kẹt ở mép màn hình.
- Buzzer không làm đứng game.
- LCD không nhấp nháy quá mức.

Kết quả cần đạt:

- Game chạy ổn định trong nhiều trận liên tiếp.

## 18. Kế hoạch thời gian đề xuất

| Giai đoạn | Nội dung | Thời lượng |
|---|---|---:|
| 1 | Cấu hình phần cứng, LCD, nút, buzzer | 2-3 ngày |
| 2 | Menu và màn hình gameplay cơ bản | 2 ngày |
| 3 | Di chuyển, nhảy, state machine | 2-3 ngày |
| 4 | Tấn công, hitbox, HP, năng lượng | 3 ngày |
| 5 | Kỹ năng và hiệu ứng | 2 ngày |
| 6 | AI và độ khó | 2-3 ngày |
| 7 | Thời gian trận đấu, game over | 1 ngày |
| 8 | Hoàn thiện sprite, âm thanh, tối ưu | 3-5 ngày |
| 9 | Kiểm thử, viết báo cáo, quay demo | 2-3 ngày |

## 19. Tiêu chí đánh giá hoàn thành

Project được xem là hoàn thành khi:

- Board chạy được game độc lập.
- LCD hiển thị menu và trận đấu.
- Người chơi điều khiển được nhân vật.
- Có ít nhất hai kiểu tấn công: đánh thường và kỹ năng.
- Có HP, năng lượng và thời gian trận đấu.
- Có trạng thái đứng/chạy/nhảy/tấn công/trúng đòn/chết.
- Có va chạm đòn đánh chính xác theo khoảng cách hoặc hitbox.
- Có AI đơn giản cho nhân vật máy.
- Có âm thanh bằng buzzer.
- Game có màn hình kết thúc và quay lại menu.

## 20. Rủi ro và cách xử lý

| Rủi ro | Cách xử lý |
|---|---|
| LCD vẽ chậm, game bị giật | Giảm FPS xuống 30, chỉ vẽ vùng thay đổi |
| Không đủ RAM cho sprite lớn | Dùng sprite nhỏ, ít frame, RGB565, hoặc hình khối đơn giản |
| Nút bị dội | Thêm debounce 20-50 ms |
| Buzzer làm chậm game | Không dùng delay dài, phát âm bằng timer/PWM |
| AI quá khó hoặc quá dễ | Điều chỉnh xác suất đánh, tốc độ phản ứng, sát thương |
| Hitbox gây sát thương nhiều lần | Thêm cờ `hasHit` cho mỗi lần ra đòn |
| Nhân vật kẹt ở mép màn hình | Clamp tọa độ X sau mỗi lần cập nhật |

## 21. Thứ tự ưu tiên khi bị thiếu thời gian

Nếu thời gian làm đồ án bị hạn chế, ưu tiên theo thứ tự:

1. LCD hiển thị sân đấu và hai nhân vật.
2. Điều khiển trái/phải/nhảy.
3. Đánh thường và trừ HP.
4. AI đơn giản biết lại gần và đánh.
5. Game over khi hết HP.
6. Năng lượng và kỹ năng.
7. Thời gian trận đấu.
8. Buzzer.
9. Sprite đẹp và hiệu ứng nâng cao.

## 22. Sản phẩm cần nộp

- Source code STM32CubeIDE.
- File `.ioc` cấu hình CubeMX.
- Video demo game chạy trên board.
- Báo cáo mô tả:
  - Mục tiêu đề tài.
  - Sơ đồ phần cứng.
  - Sơ đồ state machine.
  - Thuật toán va chạm.
  - Thuật toán AI.
  - Kết quả chạy thử.
  - Hạn chế và hướng phát triển.

