# Đặc tả sprite và background cho demo game đối kháng STM32F429

## 1. Mục tiêu của file

File này mô tả cách chuẩn bị background và sprite nhân vật cho bản demo đầu tiên của game đối kháng Naruto vs Bleach trên STM32F429.

Mục tiêu demo:

- Hiển thị được background sân đấu.
- Hiển thị được 2 nhân vật cố định trên LCD.
- Chạy vòng render ở 30 FPS.
- Sprite đủ nhẹ để phù hợp RAM/Flash của STM32F429.
- Cấu trúc sprite rõ ràng để sau này dễ thêm animation, hitbox, trạng thái và va chạm.

## 2. Giả định màn hình và FPS

### 2.1. LCD

Ưu tiên thiết kế theo màn hình ngang:

```text
LCD_WIDTH  = 320
LCD_HEIGHT = 240
```

Nếu project đang dùng màn hình dọc 240 x 320, nên cấu hình xoay ngang trong driver LCD để game có không gian di chuyển rộng hơn.

### 2.2. FPS

```c
#define GAME_FPS          30
#define FRAME_TIME_MS     33
```

Ở 30 FPS:

- Mỗi frame có khoảng 33 ms để xử lý input, logic, AI, collision và render.
- Không nên vẽ lại toàn màn hình nếu LCD chậm.
- Demo đầu tiên có thể vẽ full screen để dễ kiểm tra, sau đó tối ưu bằng dirty rectangle.

## 3. Định dạng hình ảnh khuyến nghị

### 3.1. Định dạng màu

Nên dùng RGB565 vì phù hợp LCD TFT và tiết kiệm bộ nhớ.

```text
1 pixel = 16 bit = 2 byte
```

Không nên dùng ảnh 24-bit RGB hoặc 32-bit RGBA trong runtime vì tốn bộ nhớ và cần chuyển đổi khi vẽ.

### 3.2. Transparency

Vì RGB565 không có alpha channel, dùng một màu làm màu trong suốt:

```c
#define COLOR_KEY_RGB565  0xF81F   // Magenta
```

Quy ước:

- Pixel màu magenta `0xF81F` không được vẽ ra LCD.
- Sprite phải tránh dùng màu magenta làm màu thật của nhân vật.

### 3.3. Cách lưu sprite trong code

Mỗi frame nên lưu dưới dạng mảng `const uint16_t`.

Ví dụ:

```c
const uint16_t naruto_idle_0[SPRITE_W * SPRITE_H] = {
    // RGB565 pixels
};
```

Vì là dữ liệu cố định, dùng `const` để compiler đặt vào Flash thay vì RAM.

## 4. Kích thước sprite tối ưu

### 4.1. Kích thước đề xuất cho demo

```text
Character sprite frame: 48 x 64 px
```

Lý do:

- Đủ lớn để nhìn rõ trên LCD 320 x 240.
- Không quá nặng khi lưu nhiều frame.
- Dễ căn hitbox và body box.
- Hai nhân vật đứng cùng màn hình vẫn còn khoảng trống để di chuyển.

Dung lượng mỗi frame:

```text
48 x 64 x 2 byte = 6144 byte ~= 6 KB/frame
```

Nếu mỗi nhân vật có 8 frame:

```text
6 KB x 8 = 48 KB/nhân vật
2 nhân vật ~= 96 KB
```

Mức này phù hợp hơn so với sprite lớn 64 x 96 hoặc animation quá nhiều frame.

### 4.2. Kích thước mở rộng nếu còn tài nguyên

Nếu LCD render tốt và Flash còn nhiều:

```text
Character sprite frame: 56 x 72 px
```

Tuy nhiên, không nên vượt quá 64 x 80 px trong bản đầu vì sẽ làm tăng đáng kể dung lượng và thời gian vẽ.

## 5. Tọa độ và vị trí nhân vật

### 5.1. Hệ tọa độ

```text
x tăng từ trái sang phải
y tăng từ trên xuống dưới
```

### 5.2. Mặt đất

```c
#define GROUND_Y  190
```

`GROUND_Y` là tọa độ chân nhân vật, không phải tọa độ góc trên sprite.

Khi vẽ sprite:

```c
drawX = character.x;
drawY = GROUND_Y - SPRITE_H;
```

Với sprite 48 x 64:

```text
drawY = 190 - 64 = 126
```

### 5.3. Vị trí ban đầu

```c
#define PLAYER_START_X  60
#define ENEMY_START_X   212
```

Với sprite rộng 48 px:

- Player nằm bên trái.
- Enemy nằm bên phải.
- Hai nhân vật có khoảng cách đủ để test di chuyển và tấn công sau này.

## 6. Danh sách trạng thái sprite

Demo đầu tiên chỉ cần hiển thị idle. Tuy nhiên nên đặt cấu trúc asset ngay từ đầu theo state để sau này không phải đổi lại.

### 6.1. Trạng thái bắt buộc

| State | Số frame đề xuất | Ghi chú |
|---|---:|---|
| Idle | 2 | Đứng thở nhẹ |
| Run | 4 | Chạy trái/phải |
| Jump | 5 | Đang ở trên không |
| Attack | 2 | Đánh thường |
| Skill | 2 | Tung kỹ năng |
| Hit | 1 | Bị trúng đòn |
| Dead | 1 | Gục/ngã |

Tổng:

```text
2 + 4 + 1 + 2 + 2 + 1 + 1 = 13 frame/nhân vật
```

Dung lượng nếu 48 x 64 RGB565:

```text
13 x 6144 byte ~= 78 KB/nhân vật
2 nhân vật ~= 156 KB
```

Đây là mức chấp nhận được nếu lưu trong Flash.

### 6.2. Cấu hình tối giản cho demo hiển thị đầu tiên

Để test nhanh background + sprite:

| Nhân vật | State | Frame |
|---|---|---:|
| Naruto | Idle | 1 |
| Bleach | Idle | 1 |

Dung lượng:

```text
2 frame x 6144 byte ~= 12 KB
```

Sau khi hiển thị ổn định mới thêm animation.

## 7. Tên file và tên mảng sprite

### 7.1. Quy tắc đặt tên asset gốc

Nếu lưu asset dạng ảnh trước khi convert:

```text
assets/
  sprites/
    naruto/
      naruto_idle_0.png
      naruto_idle_1.png
      naruto_run_0.png
      naruto_run_1.png
      naruto_run_2.png
      naruto_run_3.png
      naruto_jump_0.png
      naruto_attack_0.png
      naruto_attack_1.png
      naruto_skill_0.png
      naruto_skill_1.png
      naruto_hit_0.png
      naruto_dead_0.png
    bleach/
      bleach_idle_0.png
      bleach_idle_1.png
      bleach_run_0.png
      bleach_run_1.png
      bleach_run_2.png
      bleach_run_3.png
      bleach_jump_0.png
      bleach_attack_0.png
      bleach_attack_1.png
      bleach_skill_0.png
      bleach_skill_1.png
      bleach_hit_0.png
      bleach_dead_0.png
```

### 7.2. Quy tắc đặt tên mảng C

```c
naruto_idle_0
naruto_idle_1
naruto_run_0
naruto_run_1
naruto_run_2
naruto_run_3

bleach_idle_0
bleach_idle_1
bleach_run_0
bleach_run_1
bleach_run_2
bleach_run_3
```

### 7.3. File code đề xuất

```text
Core/
  Inc/
    sprite_data.h
    sprite_config.h
  Src/
    sprite_data.c
```

Trong `sprite_config.h`:

```c
#define SPRITE_W  48
#define SPRITE_H  64
#define COLOR_KEY_RGB565  0xF81F
```

Trong `sprite_data.h`:

```c
extern const uint16_t naruto_idle_0[SPRITE_W * SPRITE_H];
extern const uint16_t bleach_idle_0[SPRITE_W * SPRITE_H];
```

## 8. Cấu trúc animation

### 8.1. Animation descriptor

Nên gom các frame vào một struct để render theo state dễ hơn.

```c
typedef struct
{
    const uint16_t **frames;
    uint8_t frameCount;
    uint8_t ticksPerFrame;
    uint8_t loop;
} Animation;
```

Ở 30 FPS:

```text
1 tick = 1 frame = 33 ms
```

### 8.2. Tốc độ animation đề xuất

| State | ticksPerFrame | Tốc độ cảm nhận |
|---|---:|---|
| Idle | 15 | Đổi frame mỗi 0.5 giây |
| Run | 4 | Chạy mượt vừa phải |
| Jump | 1 | Một frame cố định |
| Attack | 3 | Đòn đánh nhanh |
| Skill | 4 | Kỹ năng rõ hơn |
| Hit | 6 | Khựng ngắn |
| Dead | 1 | Giữ frame cuối |

### 8.3. Công thức lấy frame hiện tại

```c
frameIndex = (stateTick / ticksPerFrame) % frameCount;
```

Với animation không lặp:

```c
frameIndex = stateTick / ticksPerFrame;
if (frameIndex >= frameCount)
{
    frameIndex = frameCount - 1;
}
```

## 9. Lật hướng nhân vật

Để tiết kiệm Flash, chỉ lưu sprite nhìn sang phải.

Khi nhân vật nhìn sang trái, render bằng cách lật ngang khi vẽ:

```c
if (character.facing == 1)
{
    Render_DrawSprite(x, y, sprite, SPRITE_W, SPRITE_H);
}
else
{
    Render_DrawSpriteFlipX(x, y, sprite, SPRITE_W, SPRITE_H);
}
```

Ưu điểm:

- Giảm gần một nửa dung lượng sprite.
- Không cần lưu thêm bộ frame nhìn trái.

Nhược điểm:

- Hàm vẽ flip ngang chậm hơn một chút.
- Với 48 x 64 px ở 30 FPS vẫn chấp nhận được.

## 10. Body box và hitbox gắn với sprite

### 10.1. Body box

Sprite 48 x 64 thường có vùng trong suốt xung quanh, vì vậy body box nên nhỏ hơn sprite.

Đề xuất:

```c
#define BODY_OFFSET_X  12
#define BODY_OFFSET_Y  8
#define BODY_W         24
#define BODY_H         56
```

Body box:

```text
body.x = character.x + BODY_OFFSET_X
body.y = character.y + BODY_OFFSET_Y
body.w = BODY_W
body.h = BODY_H
```

### 10.2. Hitbox đánh thường

Đề xuất cho đòn đánh thường:

```c
#define ATTACK_BOX_W  28
#define ATTACK_BOX_H  24
#define ATTACK_BOX_Y  22
```

Nếu nhìn sang phải:

```text
hitbox.x = character.x + SPRITE_W - 4
hitbox.y = character.y + ATTACK_BOX_Y
```

Nếu nhìn sang trái:

```text
hitbox.x = character.x - ATTACK_BOX_W + 4
hitbox.y = character.y + ATTACK_BOX_Y
```

### 10.3. Hitbox kỹ năng

Đề xuất cho kỹ năng dạng vùng trước mặt:

```c
#define SKILL_BOX_W  52
#define SKILL_BOX_H  32
#define SKILL_BOX_Y  16
```

Nếu muốn làm projectile sau này, vẫn có thể dùng cùng kích thước này cho vùng va chạm của projectile.

## 11. Background sân đấu

### 11.1. Kích thước background

Cho demo đầu tiên:

```text
Background: 320 x 240 px
Format: RGB565
```

Dung lượng:

```text
320 x 240 x 2 byte = 153600 byte ~= 150 KB
```

Mức này có thể lưu Flash, nhưng vẽ full background mỗi frame sẽ tốn thời gian.

### 11.2. Cách tối ưu background

Ưu tiên chia background thành các phần:

| Phần | Cách vẽ | Ghi chú |
|---|---|---|
| Sky/backdrop | Fill màu hoặc bitmap nhẹ | Ít chi tiết |
| Far object | Vẽ 1 lần hoặc bitmap nhỏ | Núi, tường, cổng |
| Ground | Fill rectangle hoặc tile lặp | Dễ xóa/vẽ lại nhân vật |

Không nên dùng background quá nhiều chi tiết ở demo đầu, vì khi xóa/vẽ lại nhân vật sẽ khó tối ưu.

### 11.3. Background tối ưu cho bản đầu

Thay vì lưu bitmap 320 x 240, có thể vẽ bằng primitive:

```text
1. Fill sky: màu xanh/xám nhạt
2. Vẽ dải xa: vài hình chữ nhật hoặc đường ngang
3. Vẽ mặt đất: rectangle màu tối
4. Vẽ line phân cách mặt đất
```

Ưu điểm:

- Gần như không tốn Flash.
- Vẽ nhanh.
- Dễ restore vùng nền khi nhân vật di chuyển.

Khi demo hiển thị ổn mới thay bằng bitmap/tile đẹp hơn.

## 12. Render sprite tối ưu

### 12.1. Không vẽ pixel trong suốt

Hàm vẽ sprite cần bỏ qua pixel `COLOR_KEY_RGB565`:

```c
if (pixel != COLOR_KEY_RGB565)
{
    LCD_DrawPixel(x + col, y + row, pixel);
}
```

### 12.2. Giảm số lần gọi DrawPixel

Nếu driver LCD hỗ trợ set window và push nhiều pixel, nên dùng:

```text
LCD_SetAddressWindow(x, y, w, h)
LCD_PushColor(pixel)
```

Tuy nhiên với color key, vẫn phải kiểm tra từng pixel. Cách tối ưu:

- Với sprite không trong suốt: push nguyên block.
- Với sprite có color key: chỉ vẽ pixel khác magenta.
- Với demo đầu, chấp nhận vẽ từng pixel để code đơn giản.

### 12.3. Dirty rectangle

Thay vì vẽ lại toàn màn hình mỗi frame:

1. Lưu vị trí cũ của nhân vật.
2. Vẽ lại background ở vùng cũ.
3. Vẽ sprite ở vị trí mới.
4. Cập nhật UI HP/energy khi giá trị thay đổi.

Vùng cần xóa nên lớn hơn sprite một chút:

```c
dirty.x = oldX - 2;
dirty.y = oldY - 2;
dirty.w = SPRITE_W + 4;
dirty.h = SPRITE_H + 4;
```

Với bản demo đầu tiên, có thể làm theo hai mức:

- Mức 1: vẽ full background + sprite mỗi frame để test logic.
- Mức 2: chuyển sang dirty rectangle để tăng FPS ổn định.

## 13. Cấu trúc render demo đầu tiên

### 13.1. Hằng số đề xuất

```c
#define LCD_WIDTH        320
#define LCD_HEIGHT       240
#define GAME_FPS         30
#define FRAME_TIME_MS    33

#define SPRITE_W         48
#define SPRITE_H         64
#define GROUND_Y         190

#define PLAYER_START_X   60
#define ENEMY_START_X    212
```

### 13.2. Trình tự render mỗi frame

```text
Render_Background()
Render_DrawSprite(player.x, player.y, naruto_idle_0)
Render_DrawSpriteFlipX(enemy.x, enemy.y, bleach_idle_0)
Render_DrawHUD()
```

Với demo chỉ hiển thị background + sprite, chưa cần update state phức tạp.

### 13.3. Pseudo code

```c
uint32_t lastFrame = 0;

while (1)
{
    uint32_t now = HAL_GetTick();

    if (now - lastFrame >= FRAME_TIME_MS)
    {
        lastFrame = now;

        Render_Background();

        Render_DrawSprite(PLAYER_START_X,
                          GROUND_Y - SPRITE_H,
                          naruto_idle_0,
                          SPRITE_W,
                          SPRITE_H);

        Render_DrawSpriteFlipX(ENEMY_START_X,
                               GROUND_Y - SPRITE_H,
                               bleach_idle_0,
                               SPRITE_W,
                               SPRITE_H);
    }
}
```

## 14. Bảng màu đề xuất cho sprite tự vẽ

Nếu chưa có sprite thật, có thể dùng sprite placeholder bằng các mảng màu đơn giản.

### 14.1. Naruto placeholder

| Thành phần | Màu RGB565 gợi ý |
|---|---|
| Tóc | `0xFFE0` vàng |
| Áo | `0xFD20` cam |
| Quần | `0x001F` xanh |
| Da | `0xFBE0` da sáng |
| Viền | `0x0000` đen |

### 14.2. Bleach placeholder

| Thành phần | Màu RGB565 gợi ý |
|---|---|
| Tóc | `0xFBE0` cam nhạt hoặc `0x0000` đen |
| Áo | `0xFFFF` trắng |
| Quần/áo choàng | `0x0000` đen |
| Kiếm | `0xC618` xám |
| Viền | `0x0000` đen |

### 14.3. Background placeholder

| Thành phần | Màu RGB565 gợi ý |
|---|---|
| Trời | `0x9E7F` xanh nhạt |
| Xa cảnh | `0x8410` xám |
| Mặt đất | `0x4208` xám đậm |
| Đường nền | `0xFFFF` trắng |

## 15. Quy trình tạo sprite thật

1. Vẽ từng frame bằng Aseprite, Piskel, GIMP hoặc công cụ pixel art khác.
2. Cố định canvas 48 x 64 px cho mọi frame.
3. Đặt nhân vật ở cùng vị trí chân trong tất cả frame.
4. Dùng nền magenta `#FF00FF` làm màu trong suốt.
5. Export PNG từng frame.
6. Convert PNG sang mảng RGB565 `const uint16_t`.
7. Kiểm tra frame trên LCD.
8. Nếu render chậm, giảm số frame hoặc giảm kích thước sprite.

## 16. Checklist cho demo hiển thị đầu tiên

- [ ] LCD chạy ngang 320 x 240.
- [ ] Có hằng số `GAME_FPS = 30`.
- [ ] Có `FRAME_TIME_MS = 33`.
- [ ] Vẽ được background.
- [ ] Vẽ được Naruto idle ở bên trái.
- [ ] Vẽ được Bleach idle ở bên phải.
- [ ] Sprite có màu trong suốt bằng color key.
- [ ] Hai nhân vật đứng đúng trên mặt đất.
- [ ] Không bị nhấp nháy nghiêm trọng.
- [ ] Code sprite lưu ở Flash bằng `const`.
- [ ] Không dùng `HAL_Delay()` trong vòng render chính.

## 17. Cấu hình khuyến nghị cho bản demo

Để tối ưu và ít rủi ro nhất, bản đầu nên dùng cấu hình sau:

```text
FPS: 30
LCD: 320 x 240 ngang
Sprite size: 48 x 64
Sprite format: RGB565
Transparency: color key 0xF81F
Frame ban đầu: 1 idle frame/nhân vật
Background ban đầu: vẽ bằng primitive, chưa dùng bitmap full màn hình
Render ban đầu: full redraw nếu vẫn đạt 30 FPS
Render tối ưu: dirty rectangle sau khi sprite hiển thị ổn
```

Đây là cấu hình cân bằng giữa độ rõ hình ảnh, tốc độ vẽ, dung lượng Flash và độ dễ triển khai trên STM32F429.

