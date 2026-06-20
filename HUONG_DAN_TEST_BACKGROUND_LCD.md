# Hướng dẫn test background trên LCD STM32F429

## 1. Các file đã thêm

```text
Core/Inc/lcd_port.h
Core/Src/lcd_port.c
Core/Inc/game_background_demo.h
Core/Src/game_background_demo.c
```

`main.c` đã gọi:

```c
GameBackgroundDemo_Init();
GameBackgroundDemo_Update();
```

Demo chạy với:

```c
#define GAME_FPS       30U
#define FRAME_TIME_MS  (1000U / GAME_FPS)
```

## 2. Background được vẽ như thế nào

Background hiện tại được vẽ bằng primitive RGB565, không dùng ảnh bitmap lớn.

Thành phần gồm:

- Bầu trời 3 dải màu.
- Mặt trời.
- Mây có dịch chuyển nhẹ để nhận biết frame đang chạy.
- Núi/cảnh xa.
- Tường sân đấu.
- Mặt đất.
- Vạch giữa sân và hiệu ứng nhỏ ở trung tâm.

Cách này nhẹ Flash hơn so với lưu ảnh full màn hình `320 x 240`.

## 3. Cấu hình LCD hiện tại

Code hiện tại đã nối `lcd_port.c` trực tiếp với thư viện ILI9341:

```c
ILI9341_Init();
ILI9341_SetRotation(SCREEN_HORIZONTAL_1);
ILI9341_DrawRectangle(...);
```

Driver ILI9341 đã được chỉnh để tự cấu hình SPI5 bằng thanh ghi, không cần `hspi5`, `MX_SPI5_Init()` hoặc `HAL_SPI_MODULE_ENABLED`.

Cấu hình chân đang dùng theo thư viện:

| Tín hiệu LCD | Chân STM32 |
|---|---|
| CS | PC2 |
| DC | PD13 |
| RST | PD12 |
| SCK | PF7, SPI5 AF5 |
| MOSI | PF9, SPI5 AF5 |

Nếu phần cứng của bạn đấu khác các chân trên, sửa trong:

```c
Core/Inc/ILI9341_STM32_Driver.h
```

Các macro cần kiểm tra:

```c
#define LCD_CS_PORT
#define LCD_CS_PIN
#define LCD_DC_PORT
#define LCD_DC_PIN
#define LCD_RST_PORT
#define LCD_RST_PIN
#define LCD_SPI_SCK_PORT
#define LCD_SPI_SCK_PIN
#define LCD_SPI_MOSI_PORT
#define LCD_SPI_MOSI_PIN
```

## 4. Kết quả mong đợi

Khi nạp code:

- Màn hình có nền sân đấu ngang `320 x 240`.
- Có bầu trời, mây, núi, tường, mặt đất.
- Mây và vạch sáng giữa sân có chuyển động nhẹ.
- Vòng update chạy theo mốc 30 FPS.

Nếu màn hình trắng/đen hoàn toàn, kiểm tra theo thứ tự:

1. `LCD_Port_Init()` đã gọi đúng init LCD chưa.
2. LCD đã bật backlight chưa.
3. Các chân CS/DC/RST/SCK/MOSI có đúng với phần cứng không.
4. Chiều màn hình có đúng `320 x 240` không.
5. Nếu màn hình đang là `240 x 320`, kiểm tra `ILI9341_SetRotation(SCREEN_HORIZONTAL_1)`.
6. Nếu build trong CubeIDE không nhận file mới, bấm chuột phải project rồi chọn Refresh, sau đó Clean Project và Build lại.
