# Tài liệu Mô tả Luật chơi & Cơ chế Đối kháng Game Bleach vs Naruto

Tài liệu này mô tả chi tiết thiết kế luật chơi, chỉ số nhân vật và cơ chế tương tác vật lý/chiến đấu trong game. Tài liệu chỉ tập trung mô tả gameplay và trải nghiệm người chơi.

---

## 1. Cơ chế HP (Máu) & Đỡ đòn (Block)
*   **Lượng máu tối đa:** Mỗi nhân vật có `100% HP`.
*   **Trạng thái bình thường:** Khi trúng đòn thường hoặc kỹ năng mà không đỡ đòn, nhân vật nhận `100% sát thương`, đồng thời bị đơ trúng đòn (Stun).
*   **Cơ chế đỡ đòn (Block):** 
    *   Nhân vật có thể chủ động đỡ đòn hướng trước mặt.
    *   Khi đỡ đòn thành công, sát thương nhận vào giảm `80%` (chỉ nhận `20% sát thương`). Nhân vật bị đơ đỡ đòn và bị đẩy lùi nhẹ.

---

## 2. Hệ thống Mana (Năng lượng) - Cơ chế 2 Thanh (50 x 2)
*   **Giới hạn tối đa:** `100 điểm`.
*   **Tích lũy Mana:**
    *   **Đánh thường trúng đối phương (Không đỡ):** Tăng **`10 điểm Mana`** mỗi hit.
    *   **Đánh trúng khi đối phương Đỡ đòn (Block):** Chỉ tăng **`1 điểm Mana`** mỗi hit.
    *   **Đánh hụt:** Không tăng Mana.
*   **Tiêu hao Mana:** 
    *   Kỹ năng đặc biệt (`SKILL`) tiêu tốn **`50 điểm Mana`** mỗi lần kích hoạt.
    *   Khi Mana dưới 50, hệ thống sẽ khóa không cho phép tung chiêu đặc biệt.

---

## 3. Hệ thống Bất tử tạm thời (I-Frames)
*   **Thời gian bảo vệ:** Sau khi bị trúng đòn, nhân vật được bất tử trong vòng **`500 miligiây` (0.5 giây)**.
*   Trong thời gian bất tử, mọi đòn tấn công của đối thủ đi qua nhân vật đều không gây sát thương hay tạo đơ đòn.

---

## 4. Cơ chế Điểm kiên định (Giáp) & Ngã sàn (Knockdown)
Điểm kiên định (Stability) thể hiện độ vững vàng của nhân vật trước sức nặng của đòn đánh.
*   **Lượng kiên định tối đa:** `100 điểm`.
*   **Quy tắc giảm điểm kiên định:**
    *   **Khi trúng đòn thường (Không đỡ):** Giảm **`20 điểm`** kiên định.
    *   **Khi đỡ đòn thường (Block):** Giảm **`0 điểm`** kiên định.
    *   **Khi trúng chiêu đặc biệt (Không đỡ):** Giảm thẳng **`100 điểm`** kiên định (gây ngã sàn ngay lập tức).
    *   **Khi đỡ chiêu đặc biệt (Block):** Giảm **`20 điểm`** kiên định.
*   **Tự động hồi phục giáp:** Nếu nhân vật không bị trúng đòn trong vòng **`1.5 giây`**, điểm kiên định tự động hồi phục về mức tối đa `100`.
*   **Trạng thái Ngã sàn (Knockdown):**
    *   Khi điểm kiên định giảm về `0`, nhân vật bị ngã xuống sàn.
    *   Trong suốt thời gian nằm sàn kéo dài **`600 miligiây` (0.6 giây)**, nhân vật được bất tử hoàn toàn.
    *   Hết 600 ms, nhân vật tự động đứng dậy ở trạng thái bình thường (Idle) và hồi phục đầy `100` điểm kiên định.

---

## 5. Thiết kế Giao diện Trực quan (HUD) trên màn hình
*   **Thanh HP (Máu):** Hiển thị ở góc trên màn hình cho P1 (màu Cyan) và CPU (màu Cam). Thanh máu tự động co ngắn và đổi sang màu đỏ cảnh báo khi lượng máu xuống dưới 30%.
*   **Thanh Mana (Năng lượng):** Nằm ngay dưới thanh máu, có màu xanh dương sáng. Ở chính giữa thanh Mana (mốc 50%) có một **vạch chia đôi 1-pixel màu đen** phân chia trực quan thành 2 vạch chiêu thức (mỗi vạch 50 Mana).
*   **Chỉ số Kiên định & Bất tử (Stability & I-Frames):** Được giữ làm **chỉ số ẩn (hidden stats)**, không hiển thị thanh trên màn hình nhằm tối ưu giao diện.
