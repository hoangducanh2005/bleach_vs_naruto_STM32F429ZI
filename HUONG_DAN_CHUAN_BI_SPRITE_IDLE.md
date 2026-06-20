# Hướng dẫn chuẩn bị sprite idle để test trên LCD STM32F429

## 1. Mục tiêu bước này

Chuẩn bị asset sprite idle của 2 nhân vật để Codex có thể code tiếp phần hiển thị nhân vật lên background ILI9341.

Mục tiêu test:

- Background tĩnh vẫn giữ nguyên.
- Vẽ được 1 sprite idle của nhân vật bên trái.
- Vẽ được 1 sprite idle của nhân vật bên phải.
- Sprite có nền trong suốt hoặc nền màu dễ tách.
- Dữ liệu sau cùng được convert sang RGB565 để chạy tốt trên STM32.

## 2. Nên lấy sprite asset kiểu nào

Ưu tiên tải:

- File `.png`.
- Nền trong suốt.
- Nhân vật dạng pixel art hoặc 2D fighting.
- Có tư thế idle rõ ràng.
- Kích thước không quá lớn.
- Nhân vật nhìn sang phải là tốt nhất, vì code có thể flip ngang khi vẽ đối thủ.

Không nên tải:

- Ảnh JPG vì không có transparency và dễ nhiễu nền.
- Ảnh quá lớn như 512x512, 1024x1024.
- Sprite sheet quá phức tạp nếu bạn chưa biết crop frame.
- Ảnh có watermark.
- Asset không rõ nguồn hoặc dùng cho thương mại nếu không có quyền.

## 3. Vấn đề bản quyền

Sprite Naruto/Bleach trên mạng thường là fan-made hoặc lấy từ game có bản quyền.

Với đồ án học tập/demo nội bộ, bạn có thể dùng để kiểm thử kỹ thuật nếu giảng viên cho phép. Nhưng nếu làm báo cáo công khai, upload GitHub hoặc demo thương mại, nên:

- Dùng sprite tự vẽ.
- Dùng asset miễn phí có license rõ ràng.
- Dùng nhân vật thay thế lấy cảm hứng thay vì copy trực tiếp Naruto/Bleach.

## 4. Từ khóa tìm kiếm gợi ý

Bạn có thể tìm trên Google bằng các cụm:

```text
naruto pixel sprite idle png transparent
naruto fighting sprite sheet idle png
bleach ichigo pixel sprite idle png transparent
ichigo fighting sprite sheet idle png
anime fighter sprite idle png transparent
free 2d fighting character sprite idle png
```

Nếu muốn tránh bản quyền anime:

```text
free pixel art ninja sprite idle png
free anime swordsman sprite idle png
free 2d fighter character sprite sheet
opengameart ninja sprite
itch.io free fighter sprite
```

## 5. Kích thước sprite yêu cầu

Cho bản test đầu tiên, dùng kích thước chuẩn:

```text
48 x 64 px
```

Nếu sprite tải về khác kích thước, resize/crop về:

```text
Canvas: 48 x 64
Nhân vật nằm giữa canvas
Chân nhân vật gần đáy canvas
Nền trong suốt hoặc màu magenta #FF00FF
```

Nếu sprite gốc quá chi tiết, có thể dùng:

```text
64 x 64 px
```

Nhưng bản đầu nên dùng `48 x 64` để nhẹ và dễ vẽ trên LCD SPI.

## 6. Quy ước đặt file

Sau khi tải/crop xong, đặt file vào project như sau:

```text
D:/final/assets/raw/naruto_idle_0.png
D:/final/assets/raw/bleach_idle_0.png
```

Nếu chưa chắc là Naruto/Bleach, vẫn đặt theo tên test:

```text
D:/final/assets/raw/player_idle_0.png
D:/final/assets/raw/enemy_idle_0.png
```

Nếu bạn có nhiều frame idle:

```text
D:/final/assets/raw/player_idle_0.png
D:/final/assets/raw/player_idle_1.png
D:/final/assets/raw/enemy_idle_0.png
D:/final/assets/raw/enemy_idle_1.png
```

Nhưng để test trước, chỉ cần mỗi nhân vật 1 frame.

## 7. Cách crop/resize bằng công cụ đơn giản

Bạn có thể dùng một trong các công cụ:

- Piskel.
- Aseprite.
- GIMP.
- Paint.NET.
- Photoshop.
- Công cụ web resize/crop PNG.

Thiết lập khi export:

```text
Format: PNG
Canvas: 48 x 64
Background: transparent
Scale interpolation: nearest neighbor nếu là pixel art
```

Nếu công cụ không giữ nền trong suốt, dùng nền magenta:

```text
#FF00FF
```

Sau này khi convert, màu này sẽ được xem là trong suốt.

## 8. Bạn cần gửi gì cho Codex

Sau khi chuẩn bị xong, bạn chỉ cần làm 1 trong 2 cách:

### Cách 1: Đặt file vào workspace

Đặt PNG vào:

```text
D:/final/assets/raw/
```

Rồi nhắn:

```text
tôi đã đặt sprite vào assets/raw, bạn convert và code hiển thị idle giúp tôi
```

### Cách 2: Gửi đường dẫn file

Nếu file nằm nơi khác, nhắn đường dẫn đầy đủ:

```text
Naruto: D:/final/assets/raw/naruto_idle_0.png
Bleach: D:/final/assets/raw/bleach_idle_0.png
```

## 9. Codex sẽ làm gì sau khi có sprite

Khi đã có file PNG, Codex sẽ:

1. Kiểm tra kích thước và transparency.
2. Resize/crop nếu cần.
3. Convert PNG sang mảng `const uint16_t` RGB565.
4. Tạo file:

```text
Core/Inc/sprite_data.h
Core/Src/sprite_data.c
Core/Inc/sprite_config.h
Core/Inc/sprite_render.h
Core/Src/sprite_render.c
```

5. Code hàm vẽ sprite có color key.
6. Vẽ player bên trái và enemy bên phải lên background.
7. Nếu cần, flip ngang enemy để hai nhân vật đối mặt nhau.

## 10. Checklist trước khi gửi sprite

- [ ] File là PNG.
- [ ] Nền trong suốt hoặc nền magenta `#FF00FF`.
- [ ] Kích thước gần `48 x 64`.
- [ ] Nhân vật không bị cắt đầu/chân.
- [ ] Chân nhân vật nằm cùng vị trí giữa các frame.
- [ ] Tên file dễ hiểu.
- [ ] Đã đặt vào `D:/final/assets/raw/`.

