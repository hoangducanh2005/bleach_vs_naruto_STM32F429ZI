# Hướng dẫn thực thi: Chế độ Người – Máy (Player vs AI)
### Game đối kháng Naruto vs Bleach trên STM32F429ZIT6 (FreeRTOS, 2 Task, đồng bộ Mutex)

Tài liệu này trình bày **quy trình thực thi**, không đi vào code — tập trung vào: làm gì trước, đo gì, quyết định dựa trên điều kiện nào, và kiểm tra ra sao ở từng bước.

---

## 1. Nguyên tắc tổng thể trước khi bắt tay làm

- AI không phải một hệ thống tách biệt — nó chỉ là **một tập lệnh điều khiển ảo**, thay thế cho joystick người chơi thứ 2. Mọi quyết định của AI đều phải đi qua đúng con đường xử lý input mà nhân vật người chơi đang dùng (di chuyển, tấn công, block, skill), không được có đường tắt riêng.
- Mọi logic trận đấu (AI, va chạm, HP, năng lượng, buzzer) nằm trong **1 task duy nhất, ưu tiên cao**. Task còn lại chỉ có nhiệm vụ vẽ lên màn hình, ưu tiên thấp hơn, không được phép chứa bất kỳ quyết định nào.
- Hai task trao đổi dữ liệu với nhau tại **đúng một điểm cuối mỗi chu kỳ xử lý**, thông qua cơ chế khóa (mutex), không trao đổi rải rác nhiều nơi.
- AI hoạt động theo **chu kỳ hành động** (ví dụ: một đòn đánh, một lần block, một lần di chuyển) chứ không theo từng khung hình rời rạc — trong suốt một chu kỳ, AI vẫn tiếp tục quan sát và chuẩn bị sẵn hành động kế tiếp, để khi chu kỳ cũ kết thúc thì chuyển ngay, không có khoảng dừng.

---

## 2. Quy trình đo lường môi trường (AI cần biết gì trước khi quyết định)

Trước mỗi lần ra quyết định, AI phải nắm được 5 nhóm thông tin sau. Thứ tự đo nên theo đúng trình tự này vì thông tin sau phụ thuộc thông tin trước:

1. **Khoảng cách giữa hai nhân vật** — xác định xem AI đang ở xa, đang ở tầm block, hay đang ở tầm ra đòn. Đây là điều kiện lọc đầu tiên, quyết định nhóm hành động nào còn khả thi.
2. **Trạng thái hiện tại của đối thủ** — đứng yên, đang di chuyển, đang nhảy, đang tấn công, đang bị trúng đòn, hay đã gục. Thông tin này quyết định AI nên phòng thủ, nên chờ, hay nên tấn công.
3. **Giai đoạn cụ thể của đòn đánh đối thủ đang thực hiện** (nếu đối thủ đang ở trạng thái tấn công) — đòn đó đang chuẩn bị ra, đang có thể trúng, hay đã ra xong và đang hồi. Đây là thông tin quan trọng nhất để phân biệt "phải phòng thủ ngay" và "có thể phản đòn".
4. **HP và năng lượng của cả hai bên** — dùng để quyết định mức độ mạo hiểm: khi năng lượng đủ và đối thủ yếu, nên dồn kỹ năng; khi bản thân yếu, nên thận trọng hơn.
5. **Thời gian còn lại của trận đấu** — chỉ cần dùng ở giai đoạn cuối trận, để quyết định giữa việc tiếp tục tấn công hay chuyển sang phòng thủ câu giờ.

**Lưu ý về cách quan sát**: AI không được đọc trạng thái đối thủ ngay tại thời điểm hiện tại một cách tức thời — cần có một độ trễ nhất định (giống thời gian phản xạ của người thật) trước khi thông tin đó được AI "nhìn thấy" và dùng để quyết định. Độ trễ này càng ngắn thì AI càng phản xạ nhanh, tương ứng với độ khó càng cao.

**Lưu ý về khối lượng dữ liệu cần lưu**: không cần lưu lại toàn bộ lịch sử quan sát, chỉ cần giữ một vài giá trị tổng hợp được cập nhật liên tục (khoảng cách trung bình gần đây, có từng thấy đối thủ tấn công/nhảy trong chu kỳ này hay không, giá trị HP thấp nhất từng thấy). Việc này giúp giảm nhẹ tài nguyên và tránh xử lý dư thừa.

---

## 3. Quy trình ra quyết định (AI chọn hành động như thế nào)

Sau khi đã có đủ thông tin quan sát, AI đánh giá **tất cả các lựa chọn hành động khả thi cùng lúc**, gán cho mỗi lựa chọn một mức độ ưu tiên, rồi chọn lựa chọn có ưu tiên cao nhất. Thứ tự ưu tiên nên tuân theo nguyên tắc sau (từ cao xuống thấp):

1. **Phản đòn (Punish)** — được ưu tiên cao nhất, chỉ áp dụng khi đối thủ đang ở giai đoạn hồi chiêu (đã ra đòn xong, chưa kịp hồi phục) và trong tầm đánh. Đây là cơ hội "chắc ăn" nhất nên luôn được xét trước.
2. **Đánh chặn trên không (Anti-air)** — áp dụng khi đối thủ đang trong quá trình nhảy và đang ở khoảng giữa quỹ đạo nhảy (không phải lúc vừa bật nhảy hay vừa sắp chạm đất), vì đây là lúc đối thủ khó phản ứng nhất.
3. **Phòng thủ (Block)** — áp dụng khi đối thủ đang trong giai đoạn đòn đánh thật sự có thể trúng (không phải lúc đang chuẩn bị ra đòn), và ở trong tầm bị ảnh hưởng.
4. **Dùng kỹ năng (Skill)** — áp dụng khi đủ năng lượng và trong tầm sử dụng; mức độ ưu tiên cần tăng thêm khi HP đối thủ đã xuống thấp, để dồn kỹ năng kết liễu thay vì dùng tùy tiện.
5. **Tung chuỗi đòn (Combo)** — áp dụng khi đã ở trong tầm đánh thường, là lựa chọn mặc định khi không có cơ hội đặc biệt nào ở trên.
6. **Tiến lại gần hoặc lùi ra xa** — chỉ được chọn khi không có lựa chọn nào ở trên phù hợp, dùng để điều chỉnh cự ly cho phù hợp với tầm tấn công.

Sau khi đã xác định lựa chọn có ưu tiên cao nhất, cần thêm **một bước xác suất cuối cùng** trước khi thực sự thực hiện — nghĩa là ngay cả khi một hành động được xếp ưu tiên cao nhất, AI vẫn có thể không thực hiện nó (theo một tỷ lệ phần trăm tùy độ khó), để tránh cảm giác AI phản ứng máy móc, luôn đúng 100% mỗi khi có cơ hội.

**Điều chỉnh theo giai đoạn cuối trận**: khi thời gian còn lại của trận sắp hết, cần bổ sung một lớp điều chỉnh riêng: nếu AI đang có lợi thế về HP thì nên tăng thiên hướng phòng thủ; nếu đang bất lợi thì nên tăng thiên hướng tấn công liều lĩnh hơn so với bình thường.

---

## 4. Quy trình vận hành theo chu kỳ hành động (không phải theo từng khung hình)

Với mỗi hành động AI đang thực hiện (một đòn đánh, một lần block, một lần di chuyển), cần tuân theo trình tự sau:

1. Xác định độ dài của chu kỳ hành động hiện tại — độ dài này khác nhau tùy loại hành động (đòn nhẹ ngắn, kỹ năng dài, dash rất ngắn) và nên dựa theo thời gian chuẩn bị + thời gian ra đòn + thời gian hồi chiêu của chính hành động đó.
2. Trong suốt chu kỳ này, AI vẫn tiếp tục đo lường môi trường liên tục ở bước 2, dù đang "bận" thực hiện hành động hiện tại — không được dừng quan sát chỉ vì đang thực hiện hành động khác.
3. Khi chu kỳ đã trôi qua một tỷ lệ phần trăm nhất định (ví dụ khoảng 55–85% tùy độ khó, độ khó cao thì tỷ lệ này thấp hơn để có nhiều thời gian cân nhắc hơn), AI tiến hành đánh giá và chốt hành động tiếp theo — nhưng chỉ chốt, chưa thực hiện ngay.
4. Khi chu kỳ hiện tại kết thúc, hành động đã được chốt từ bước 3 được thực hiện ngay lập tức, không có độ trễ chờ thêm; đồng thời bắt đầu một chu kỳ mới cho hành động vừa được thực hiện.
5. Nếu vì lý do nào đó AI chưa kịp chốt được hành động tiếp theo (ví dụ chưa đủ dữ liệu quan sát tối thiểu), cần có một hành động dự phòng an toàn (thường là di chuyển giữ cự ly) để tránh AI đứng im không phản ứng gì.
6. Nếu trong lúc đang thực hiện chu kỳ mà AI bị đối thủ đánh trúng (chuyển sang trạng thái trúng đòn) hoặc gục, toàn bộ chu kỳ đang diễn ra phải được **hủy ngay lập tức**, không chờ hết chu kỳ cũ — vì lúc này AI không còn quyền chủ động để tiếp tục kế hoạch đã định.

---

## 5. Cách phân biệt 3 mức độ khó — chỉ thay đổi tham số, không thay đổi quy trình

Ba mức độ khó (Dễ, Trung bình, Khó) không nên có 3 bộ quy trình khác nhau, mà dùng chung một quy trình ở mục 2–4, chỉ khác nhau ở các con số cấu hình sau:

- **Độ trễ trước khi AI "nhìn thấy" hành động của đối thủ**: mức Dễ có độ trễ dài nhất (phản ứng chậm, dễ bỏ lỡ cơ hội), mức Khó có độ trễ ngắn nhất (gần như phản ứng tức thì).
- **Tỷ lệ phần trăm chu kỳ hành động cần trôi qua trước khi bắt đầu ra quyết định tiếp theo**: mức Dễ chờ đến gần cuối chu kỳ mới bắt đầu nghĩ (dễ ra quyết định vội vàng, sai sót), mức Khó bắt đầu nghĩ sớm hơn nhiều trong chu kỳ (có nhiều thời gian cân nhắc, quyết định chuẩn xác hơn).
- **Số lượng thông tin quan sát tối thiểu cần có trước khi được phép ra quyết định**: mức Dễ chỉ cần rất ít, mức Khó cần nhiều hơn để đảm bảo quyết định chính xác.
- **Tỷ lệ phần trăm thực sự thực hiện mỗi loại hành động** (phòng thủ, phản đòn, dùng kỹ năng, tung chuỗi đòn, đánh chặn trên không): tăng dần từ Dễ đến Khó.
- **Sai số khi tính toán vị trí di chuyển**: mức Dễ có sai số lớn (di chuyển không chính xác, dễ bị hụt tầm), mức Khó có sai số rất nhỏ (di chuyển chuẩn xác).

Khi cần điều chỉnh độ khó, chỉ cần thay đổi bảng tham số này, không cần sửa lại luồng xử lý đã xây ở mục 2–4.

---

## 6. Quy trình phối hợp giữa hai task (Xử lý trận đấu và Hiển thị)

1. Task xử lý trận đấu chạy theo chu kỳ thời gian cố định tuyệt đối (không bị trôi lệch dù mỗi vòng tốn thời gian xử lý nhiều hay ít khác nhau); đây là task duy nhất được phép chứa toàn bộ logic AI, va chạm, HP, năng lượng, và kích hoạt âm thanh.
2. Task hiển thị chạy độc lập, với chu kỳ có thể chậm hơn task xử lý trận đấu (không cần khớp tuyệt đối), và tuyệt đối không được chứa bất kỳ quyết định logic nào — chỉ đọc dữ liệu đã được chuẩn bị sẵn rồi vẽ ra màn hình.
3. Tại cuối mỗi chu kỳ xử lý, task xử lý trận đấu sao chép một bản dữ liệu tóm tắt (vị trí, HP, năng lượng, trạng thái, thời gian trận đấu còn lại) sang một vùng dữ liệu dùng chung, được bảo vệ bằng cơ chế khóa.
4. Task hiển thị, ở đầu mỗi chu kỳ vẽ của nó, cũng thông qua cùng cơ chế khóa để đọc bản sao dữ liệu đó, rồi giải phóng khóa ngay lập tức trước khi bắt đầu vẽ — quá trình vẽ (có thể tốn thời gian) không được diễn ra trong lúc đang giữ khóa.
5. Nếu việc lấy khóa không thành công trong một khoảng thời gian rất ngắn cho phép, task hiển thị nên bỏ qua và dùng lại dữ liệu của chu kỳ trước, thay vì chờ đợi vô thời hạn — tránh làm chậm trễ toàn bộ hệ thống chỉ vì một lần tranh chấp khóa.
6. Task xử lý trận đấu cần được thiết lập mức ưu tiên cao hơn task hiển thị, để đảm bảo timing của các đòn đánh và quyết định AI không bị ảnh hưởng bởi thời gian vẽ màn hình.

---

## 7. Trình tự triển khai khuyến nghị (làm từng bước, không làm dồn một lúc)

1. Dựng khung hai task hoạt động ổn định với chu kỳ thời gian đúng như thiết kế, xác nhận bằng cách đo thời gian thực tế trước khi thêm bất kỳ logic trận đấu nào.
2. Đưa toàn bộ xử lý input và trạng thái của nhân vật người chơi vào task xử lý trận đấu, kiểm tra hiển thị hoạt động mượt mà khi chưa có đối thủ AI.
3. Cho nhân vật thứ hai hoạt động bằng một joystick thứ hai thật (chế độ hai người), để xác nhận toàn bộ cơ chế va chạm, HP, năng lượng, và âm thanh hoạt động đúng trước khi đưa AI vào.
4. Thay thế joystick thứ hai bằng AI ở mức độ khó Dễ trước tiên — vì đây là mức đơn giản nhất, dễ quan sát và gỡ lỗi hành vi.
5. Kiểm tra riêng từng tình huống quan trọng: AI có phản đòn đúng lúc đối thủ hở sau chuỗi đòn hay không, AI có đánh chặn đúng lúc đối thủ nhảy vào hay không, AI có phản ứng hợp lý khi bị dồn ép liên tục hay không.
6. Sau khi mức Dễ hoạt động đúng như kỳ vọng, chuyển sang thử nghiệm mức Trung bình và Khó bằng cách chỉ thay đổi bảng tham số, quan sát xem cảm giác chơi có tăng độ khó hợp lý hay không.
7. Cuối cùng mới xây dựng menu chọn nhân vật và chọn độ khó, gắn vào phần khởi tạo trận đấu, sau khi toàn bộ hành vi AI đã được xác nhận ổn định.
