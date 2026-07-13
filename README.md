# 🎮 Game Đối Kháng Bleach vs Naruto - STM32F429ZIT6

Dự án game đối kháng **Bleach vs Naruto** được lập trình bằng ngôn ngữ C, chạy trên hệ thống vi điều khiển **STM32F429ZIT6** tích hợp màn hình **TFT LCD ILI9341** và hệ điều hành thời gian thực **FreeRTOS**.

---

## 🛠️ Yêu cầu Phần cứng & Thiết bị ngoại vi
*   **MCU:** STM32F429ZIT6 (ARM Cortex-M4, 2 MB Flash, 256 KB RAM).
*   **Màn hình:** TFT LCD ILI9341 (Độ phân giải 320x240, giao tiếp SPI/Parallel).
*   **Âm thanh:** Còi chíp thụ động (Passive Buzzer) được điều khiển qua xung PWM (Timer).
*   **Điều khiển:** Nút nhấn (Buttons) hoặc Joystick rời.

---

## ✨ Tính năng chính của Dự án
1.  **Hệ thống Nhân vật (6 nhân vật cố định):**
    *   Tích hợp các nhân vật nổi tiếng như: Sasuke, Ichigo, Vizard Ichigo, Naruto, Rukia...
    *   Mỗi nhân vật có sprite sheets hoạt ảnh hoạt họa riêng biệt.
2.  **Bộ kỹ năng & Điều khiển:**
    *   Di chuyển trái/phải, nhảy (Jump), lướt nhanh (Dash).
    *   Đỡ đòn (Block) hướng trước mặt giảm 80% sát thương.
    *   Tấn công thường: Combo chuỗi tấn công (khi nhấn nút tấn công liên tục 3 lần) và tấn công theo hướng.
    *   Kỹ năng đặc biệt (Skill) tiêu tốn năng lượng (Mana).
3.  **Vật lý & Phát hiện va chạm (Hitbox/Hurtbox):**
    *   Tính toán trọng lực (`COMBAT_GRAVITY`), lực nhảy (`COMBAT_JUMP_FORCE`) và vận tốc rơi tối đa.
    *   Quét vùng va chạm hình hộp (AABB Box Collision) thời gian thực giữa Hitbox của đòn đánh và Hurtbox của mục tiêu.
4.  **Chế độ chơi đa dạng:**
    *   **Người vs Máy (Player vs AI):** Tích hợp AI thông minh với 3 mức độ khó (Dễ, Trung bình, Khó) điều chỉnh qua thời gian phản xạ và độ chính xác của AI.
    *   **Người vs Người (PvP):** Chế độ 2 người chơi qua 2 bộ điều khiển.
5.  **Giao diện HUD đẹp mắt & Tối ưu hiển thị:**
    *   Hiển thị HP (màu sắc động chuyển sang đỏ khi máu thấp) và Mana (2 thanh chia đôi bởi vạch 1-pixel đen).
    *   Tích hợp bộ đếm ngược thời gian trận đấu (mặc định 60 giây).
    *   **Tối ưu hóa Render:** Sử dụng thuật toán **Dirty Rectangles** (chỉ vẽ lại vùng màn hình có sự thay đổi vị trí của các nhân vật/hiệu ứng đạn) giúp tăng FPS lên mức tối đa, chống nhấp nháy trên màn hình LCD.
6.  **Hệ thống âm thanh Buzzer:**
    *   Phát hiệu ứng âm thanh (SFX) thông qua Passive Buzzer sử dụng bộ Timer xuất xung PWM.
    *   Âm thanh phản hồi trực quan theo sự kiện: Ra chiêu (tấn công), Nhảy, Bị trúng đòn, và Giai điệu khi kết thúc trận đấu (Game Over).

---

## ⚙️ Kiến trúc Hệ thống (FreeRTOS)
Dự án được phân chia thành **2 Task chính** đồng bộ qua khóa **Mutex** để đảm bảo tính thời gian thực:
*   **Task 1: Xử lý Trận đấu (Mức ưu tiên cao):**
    *   Tính toán logic vật lý, trạng thái nhân vật (Idle, Run, Jump, Attack, Hit, Dead).
    *   Quét va chạm, cập nhật HP/Mana, thực thi AI cho Robot, xuất âm thanh ra Buzzer.
*   **Task 2: Hiển thị giao diện - Render (Mức ưu tiên thấp):**
    *   Nhận bản sao dữ liệu được bảo vệ bằng Mutex từ Task 1 và render nhanh lên LCD ILI9341.

---

## 📊 Cơ chế Chỉ số & Luật chơi (Combat Specification)
*   **Máu (HP):** Mặc định 100%. Máu dưới 30% còi báo/thanh máu đổi màu cảnh báo.
*   **Đỡ đòn (Block):** Nhận 20% sát thương thông thường. Bị đẩy lùi nhẹ.
*   **Năng lượng (Mana):** Giới hạn tối đa 100 điểm. Đánh thường trúng tăng 10 mana, đối thủ đỡ đòn chỉ tăng 1 mana. Tung Skill tốn **50 mana**.
*   **Bất tử tạm thời (I-Frames):** Sau khi bị trúng đòn, nhân vật bất tử trong **500 ms** để tránh bị dồn combo liên tục.
*   **Điểm kiên định (Stability - Giáp ẩn):** Tối đa 100. Trúng đòn thường giảm 20. Khi về 0, nhân vật rơi vào trạng thái **Ngã sàn (Knockdown)** bất tử trong **600 ms** trước khi tự đứng dậy hồi phục lại giáp.

---

## 📂 Cấu trúc thư mục chính
```text
├── Core/
│   ├── Inc/             # Khai báo Header (.h) của game và driver LCD
│   └── Src/             # Mã nguồn C (.c) xử lý logic, nhân vật, render
│       ├── main.c       # Điểm bắt đầu chương trình
│       ├── battle_demo.c# Logic sàn đấu và vòng lặp game chính
│       ├── combat_actor.c# Logic hành vi, vật lý nhân vật
│       ├── combat_rules.c# Luật chơi (Sát thương, mana, giáp kiên định)
│       └── *moveset.c   # Dữ liệu ảnh/sprite nhúng dưới dạng mảng C
├── docs/                # Layout UI và tài liệu hướng dẫn vận hành AI
├── tools/               # Các script Python hỗ trợ cắt ghép ảnh và chuyển đổi sang mảng C
└── .gitignore           # Cấu hình bỏ qua các file build tạm thời khi push git
```

---

## 🚀 Hướng dẫn Biên dịch & Nạp chương trình
1.  **Mở dự án:** Sử dụng phần mềm **STM32CubeIDE** import thư mục gốc của dự án.
2.  **Lưu ý quan trọng về bộ nhớ Flash (2 MB giới hạn):**
    *   Do các file moveset nhân vật (`sasuke_moveset.c`, `ichigo_moveset.c`, `vizard_moveset.c`, `naruto_moveset.c`) chứa mảng dữ liệu ảnh hằng số rất lớn, biên dịch tất cả cùng lúc sẽ gây tràn bộ nhớ Flash của chip.
    *   **Giải pháp:** Click chuột phải vào những file moveset của nhân vật hoặc file demo không cần test ở thời điểm hiện tại trong thư mục `Core/Src` -> Chọn **Resource Configurations** -> **Exclude from Build...** -> Tích chọn tất cả cấu hình biên dịch -> **OK**.
3.  **Build dự án:** Nhấn biểu tượng Búa (Build) hoặc tổ hợp phím `Ctrl + B` trên STM32CubeIDE.
4.  **Nạp chương trình:** Kết nối board STM32F429 Discovery thông qua cổng USB ST-LINK, nhấn biểu tượng Run hoặc Debug để nạp chương trình vào chip.
