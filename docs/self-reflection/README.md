# Self-reflection register

Các file dùng chung cho hiện tại và tương lai, ghi nhận hai loại vấn đề khi làm
việc trên series POC ESP32:

- `mistakes.md` — sai lầm **tự gây ra** (của người thực hiện: nhầm đường dẫn,
  đọc sai bằng chứng, cấu hình sai...).
- `tool-limits.md` — **giới hạn của công cụ bên ngoài** (Wokwi, ngrok,
  PlatformIO, ESP32 Arduino core...) — không do code của chúng ta gây nên.

## Cấu trúc

Mỗi file chứa các **mục được đánh số thứ tự** (`## 1.`, `## 2.`, ...). Khi có
vấn đề mới, **thêm mục số tiếp theo xuống cuối file**, không chèn giữa các mục
cũ (để số thứ tự ổn định dùng làm tham chiếu).

Mỗi mục có đúng 4 mục con:

| Mục | Nội dung |
|---|---|
| `desc` | Mô tả chung vấn đề: triệu chứng, bằng chứng đã thu được. |
| `root-cause` | Lý do gốc (không phải cách sửa). |
| `pot-sol` | Liệt kê các hướng giải quyết tiềm năng (chưa chọn). |
| `selected-choice` | Hướng đã chọn (hoặc "chưa chọn"), kèm lý do ngắn. |

Ghi ngày `YYYY-MM-DD` vào tiêu đề mỗi mục.
