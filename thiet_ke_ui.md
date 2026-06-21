# Thiết kế màn hình UI cho game đối kháng Naruto vs Bleach trên STM32F429I

## 1. Mục tiêu UI

File này mô tả các màn hình cần có cho game đối kháng Naruto vs Bleach chạy trên STM32F429I với LCD, joystick/nút và buzzer.

Yêu cầu chính của project:

- Hai nhân vật cố định: Naruto và Ichigo.
- Di chuyển trái/phải, nhảy, đánh thường, dùng kỹ năng.
- Quản lý HP và năng lượng.
- Có trạng thái: đứng, chạy, nhảy, tấn công, trúng đòn, chết.
- Phát hiện khoảng cách hoặc vùng va chạm khi tấn công.
- Chế độ người-máy với AI luật đơn giản.
- Có giới hạn thời gian trận đấu.
- Có âm thanh khi đánh, trúng đòn và kết thúc.
- Có menu chọn nhân vật hoặc mức độ khó.

## 2. Giả định màn hình

Ưu tiên dùng LCD ngang:

```c
#define UI_SCREEN_WIDTH   320
#define UI_SCREEN_HEIGHT  240
```

Tọa độ:

```text
x tăng từ trái sang phải
y tăng từ trên xuống dưới
```

Khu vực màn hình:

```text
0..39      : HUD
40..181    : khu vực nhân vật / hiệu ứng / projectile
182..239   : mặt đất
```

Mặt đất gameplay:

```c
#define GAME_GROUND_Y 190
```

## 3. Danh sách màn hình

```text
SCREEN_SPLASH
SCREEN_MAIN_MENU
SCREEN_CHARACTER_SELECT
SCREEN_DIFFICULTY_SELECT
SCREEN_GAMEPLAY
SCREEN_ROUND_END
SCREEN_GAME_OVER
SCREEN_PAUSE       // mở rộng nếu còn thời gian
```

Với bản tối thiểu, có thể chỉ cần:

```text
SCREEN_MAIN_MENU
SCREEN_DIFFICULTY_SELECT
SCREEN_GAMEPLAY
SCREEN_GAME_OVER
```

## 4. Luồng màn hình

```text
SPLASH
  -> MAIN_MENU

MAIN_MENU
  -> GAMEPLAY nếu chọn Start
  -> DIFFICULTY_SELECT nếu chọn Difficulty
  -> CHARACTER_SELECT nếu chọn Character

CHARACTER_SELECT
  -> MAIN_MENU sau khi chọn Naruto/Ichigo

DIFFICULTY_SELECT
  -> MAIN_MENU sau khi chọn Easy/Normal/Hard

GAMEPLAY
  -> GAME_OVER khi HP = 0 hoặc hết thời gian

GAME_OVER
  -> MAIN_MENU khi nhấn Start/Confirm
```

## 5. Màn hình Splash

### 5.1. Mục đích

Hiển thị nhanh tên game khi khởi động.

### 5.2. Thành phần

```text
NARUTO vs BLEACH
STM32 Fighting Demo
```

### 5.3. Hành vi

- Tự chuyển sang `MAIN_MENU` sau 1-2 giây.
- Có thể bỏ qua nếu nhấn Confirm.

### 5.4. Gợi ý layout

```text
+--------------------------------+
|                                |
|         NARUTO vs BLEACH       |
|       STM32 Fighting Demo      |
|                                |
+--------------------------------+
```

## 6. Màn hình Main Menu

### 6.1. Mục đích

Cho người chơi bắt đầu game hoặc vào các menu phụ.

### 6.2. Thành phần

```text
NARUTO vs BLEACH

> START
  DIFFICULTY
  CHARACTER
```

### 6.3. Điều khiển

| Input | Hành động |
|---|---|
| Up/Down | Di chuyển lựa chọn |
| Confirm | Chọn mục |
| Back | Quay lại nếu đang ở menu phụ |

### 6.4. Layout đề xuất

```text
+--------------------------------+
|        NARUTO vs BLEACH        |
|                                |
|           > START              |
|             DIFFICULTY         |
|             CHARACTER          |
|                                |
| Difficulty: Normal             |
+--------------------------------+
```

### 6.5. Ghi chú tối ưu

- Nền menu nên đơn giản, không animation nặng.
- Chỉ redraw khi cursor thay đổi.
- Có thể dùng text trắng trên nền tối hoặc nền sân đấu đã vẽ mờ.

## 7. Màn hình Character Select

### 7.1. Mục đích

Cho người chơi chọn nhân vật. Vì đề tài ưu tiên hai nhân vật cố định, UI này có thể rất đơn giản.

### 7.2. Thành phần

```text
SELECT CHARACTER

> Naruto
  Ichigo
```

Nếu người chơi chọn Naruto:

```text
Player: Naruto
CPU   : Ichigo
```

Nếu người chơi chọn Ichigo:

```text
Player: Ichigo
CPU   : Naruto
```

### 7.3. Layout đề xuất

```text
+--------------------------------+
|       SELECT CHARACTER         |
|                                |
|    [Naruto Sprite] [Ichigo]    |
|       > Naruto     Ichigo      |
|                                |
| CPU will use the other fighter |
+--------------------------------+
```

### 7.4. Bản tối thiểu

Nếu thiếu thời gian, bỏ màn hình này và cố định:

```text
Player: Ichigo
CPU   : Naruto
```

## 8. Màn hình Difficulty Select

### 8.1. Mục đích

Chọn mức AI.

### 8.2. Thành phần

```text
SELECT DIFFICULTY

> Easy
  Normal
  Hard
```

### 8.3. Ảnh hưởng gameplay

| Mức | AI reaction | Tần suất đánh | Tần suất dùng skill |
|---|---:|---:|---:|
| Easy | chậm | thấp | thấp |
| Normal | vừa | vừa | vừa |
| Hard | nhanh | cao | cao |

### 8.4. Layout đề xuất

```text
+--------------------------------+
|       SELECT DIFFICULTY        |
|                                |
|            EASY                |
|          > NORMAL              |
|            HARD                |
|                                |
+--------------------------------+
```

## 9. Màn hình Gameplay

### 9.1. Mục đích

Đây là màn hình chính của trận đấu.

### 9.2. Thành phần bắt buộc

- Background sân đấu.
- Player sprite.
- CPU sprite.
- HP player.
- HP CPU.
- Energy player.
- Energy CPU.
- Thời gian trận đấu.
- Projectile/skill effect.
- Trạng thái game: fighting, hit, dead, timeout.

### 9.3. Layout tổng thể

```text
+--------------------------------+
| P1 HP [========]  60  [========] CPU HP |
| P1 EN [====    ]      [===     ] CPU EN |
|                                |
|                                |
|    Ichigo           Naruto     |
|                                |
|          projectile/effect     |
|________________________________|
```

Vì LCD chỉ 320x240, HUD nên gọn:

```text
HP trái:    x=8,   y=6,  w=100, h=8
HP phải:    x=212, y=6,  w=100, h=8
Time:       x=148, y=4,  w=24,  h=16
EN trái:    x=8,   y=20, w=80,  h=5
EN phải:    x=232, y=20, w=80,  h=5
```

### 9.4. HUD chi tiết

#### HP bar

Màu đề xuất:

```text
HP nền     : xám đậm
HP còn lại : xanh lá
HP thấp    : đỏ nếu HP < 25%
Viền       : trắng hoặc đen
```

#### Energy bar

Màu đề xuất:

```text
Energy nền : xám đậm
Energy     : xanh dương/cyan
Đủ skill   : nhấp nháy nhẹ hoặc đổi sang vàng
```

#### Timer

```text
90, 89, 88, ...
```

Khi còn dưới 10 giây:

- Đổi màu đỏ.
- Buzzer bíp ngắn mỗi giây nếu muốn.

### 9.5. Vùng gameplay

```text
Player start x = 50..70
CPU start x    = 220..240
GROUND_Y       = 190
```

Sprite được căn theo chân:

```c
drawY = GROUND_Y - spriteHeight;
```

Không căn theo góc trên cố định, vì mỗi animation có thể cao khác nhau.

### 9.6. Render tối ưu

Không redraw toàn màn hình mỗi frame.

Ưu tiên:

```text
1. Background tĩnh vẽ một lần.
2. Sprite dùng draw-diff hoặc dirty rectangle nhỏ.
3. Projectile erase vùng cũ rồi draw vùng mới.
4. HUD chỉ redraw khi HP/energy/time thay đổi.
```

## 10. Màn hình Round End

### 10.1. Mục đích

Hiển thị kết quả nhanh trước khi vào `GAME_OVER` hoặc quay lại menu.

### 10.2. Thành phần

```text
YOU WIN
YOU LOSE
DRAW
TIME UP
```

### 10.3. Hành vi

- Phát âm thanh kết thúc.
- Dừng input gameplay.
- Sau 2 giây chuyển sang `GAME_OVER`.

## 11. Màn hình Game Over

### 11.1. Thành phần

```text
GAME OVER

Winner: Ichigo
Press Start
```

Hoặc:

```text
YOU WIN
Press Start
```

### 11.2. Điều khiển

| Input | Hành động |
|---|---|
| Confirm/Start | Quay về Main Menu |

### 11.3. Layout đề xuất

```text
+--------------------------------+
|                                |
|            YOU WIN             |
|                                |
|       Press Start to Menu      |
|                                |
+--------------------------------+
```

## 12. Màn hình Pause

Màn hình này là mở rộng, không bắt buộc.

### 12.1. Thành phần

```text
PAUSED

> Resume
  Main Menu
```

### 12.2. Hành vi

- Dừng logic game.
- Dừng AI.
- Dừng projectile.
- Có thể giữ nguyên frame màn hình và vẽ overlay tối.

## 13. State UI tổng quát

```c
typedef enum
{
    SCREEN_SPLASH,
    SCREEN_MAIN_MENU,
    SCREEN_CHARACTER_SELECT,
    SCREEN_DIFFICULTY_SELECT,
    SCREEN_GAMEPLAY,
    SCREEN_ROUND_END,
    SCREEN_GAME_OVER,
    SCREEN_PAUSE
} ScreenState;
```

Game context:

```c
typedef struct
{
    ScreenState screen;
    uint8_t selectedMenuItem;
    uint8_t selectedCharacter;
    uint8_t difficulty;
    uint8_t winner;
    uint16_t matchTime;
} UIContext;
```

## 14. API UI đề xuất

```c
void UI_Init(void);
void UI_Update(void);
void UI_Render(void);

void UI_SetScreen(ScreenState screen);
void UI_RenderMainMenu(void);
void UI_RenderDifficultyMenu(void);
void UI_RenderCharacterSelect(void);
void UI_RenderGameplayHUD(void);
void UI_RenderGameOver(void);
```

Với gameplay:

```c
void UI_DrawHPBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t hp, uint16_t maxHp);
void UI_DrawEnergyBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t energy, uint16_t maxEnergy);
void UI_DrawTimer(uint16_t seconds);
```

## 15. Bảng màu UI RGB565

```c
#define UI_COLOR_BLACK       0x0000
#define UI_COLOR_WHITE       0xFFFF
#define UI_COLOR_RED         0xF800
#define UI_COLOR_GREEN       0x07E0
#define UI_COLOR_BLUE        0x001F
#define UI_COLOR_CYAN        0x07FF
#define UI_COLOR_YELLOW      0xFFE0
#define UI_COLOR_ORANGE      0xFD20
#define UI_COLOR_DARK_GRAY   0x4208
#define UI_COLOR_GRAY        0x8410
```

## 16. Input UI

Nên chuyển input thô thành input event:

```c
typedef struct
{
    uint8_t upPressed;
    uint8_t downPressed;
    uint8_t leftPressed;
    uint8_t rightPressed;
    uint8_t confirmPressed;
    uint8_t backPressed;
    uint8_t attackPressed;
    uint8_t skillPressed;
    uint8_t jumpPressed;
} InputState;
```

Menu dùng:

```text
up/down/confirm/back
```

Gameplay dùng:

```text
left/right/jump/attack/skill
```

## 17. Thứ tự ưu tiên triển khai UI

Nếu thời gian hạn chế, làm theo thứ tự:

1. Gameplay HUD: HP, energy, timer.
2. Game over đơn giản.
3. Main menu Start.
4. Difficulty menu.
5. Character select.
6. Pause.
7. Splash/hiệu ứng đẹp.

## 18. Checklist hoàn thành UI

- [ ] Có màn hình menu chính.
- [ ] Có chọn độ khó.
- [ ] Có thể vào gameplay.
- [ ] Gameplay có HP hai bên.
- [ ] Gameplay có energy hai bên.
- [ ] Gameplay có timer.
- [ ] Có màn hình thắng/thua/hòa.
- [ ] Có thể quay lại menu sau trận.
- [ ] UI không redraw toàn màn hình liên tục.
- [ ] Text/HUD không che nhân vật.
- [ ] Các thanh HP/energy cập nhật đúng khi giá trị thay đổi.

