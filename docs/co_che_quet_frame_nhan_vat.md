# Co che quet frame nhan vat

Tai lieu nay mo ta cach tach frame tu sprite sheet nhan vat nhu Ichigo va Naruto, dong thoi giai thich cach tinh thoi gian chay het cac frame trong mot state animation.

## 1. Dau vao

Dau vao la mot sprite sheet PNG co nhieu pose/frame nam tren cung mot anh lon.

Vi du:

- `DS _ DSi - Jump Ultimate Stars - Fighters - Ichigo Kurosaki.png`
- `DS _ DSi - Jump Ultimate Stars - Fighters - Naruto Uzumaki.png`

Dac diem quan trong cua cac sheet nay:

- Nen la mau phang: RGB `(128, 128, 255)`.
- Sprite nhan vat va effect co mau khac nen.
- Cac frame duoc sap xep theo hang, doc tu trai sang phai, tu tren xuong duoi.

## 2. Nguyen ly tach nen

Script coi moi pixel co mau `(128, 128, 255)` la nen.

Neu pixel khac mau nen thi duoc xem la pixel cua sprite.

Ket qua:

```text
pixel == (128,128,255) -> transparent
pixel != (128,128,255) -> foreground
```

Khi xuat frame PNG rieng le, pixel nen se duoc doi thanh alpha `0`, tuc la trong suot.

## 3. Tao mask foreground

Script tao mot anh mask trang den:

```text
foreground pixel -> 255
background pixel -> 0
```

Sau do dung phep `MaxFilter` de noi cac pixel gan nhau lai thanh mot cum lon hon.

Ly do can buoc nay:

- Toc nhan vat, kiem, vet chem, lua, shadow co the bi tach thanh cac cum nho.
- Neu khong noi cum, mot frame co the bi cat thanh nhieu manh rieng.
- `MaxFilter` giup gom cac chi tiet gan nhau thanh mot frame day du.

Trong script hien tai:

```python
DILATE_SIZE = 11
```

Gia tri nay nghia la mask duoc noi rong trong vung lan can 11 pixel.

## 4. Tim connected components

Sau khi co mask da noi cum, script chay connected component.

No quet tung pixel trong mask:

1. Gap pixel foreground chua tham.
2. Dung hang doi BFS de lan sang cac pixel foreground lan can.
3. Ghi lai bounding box cua ca cum:

```text
min_x, min_y, max_x, max_y
```

Moi cum lon tuong ung voi mot frame nhan vat.

## 5. Loc nhieu va watermark

Khong phai component nao cung la frame hop le. Script loc bo:

- Cum qua nho.
- Bounding box qua nho.
- Bounding box qua lon bat thuong.
- Vung watermark o goc duoi phai cua sprite sheet.

Mot so tham so dang dung:

```python
MIN_PIXELS = 18
MIN_BBOX_AREA = 120
MAX_FRAME_WIDTH = 160 hoặc 170
MAX_FRAME_HEIGHT = 160 hoặc 170
```

Voi Naruto, gioi han lon hon mot chut vi co nhieu effect:

```python
MAX_FRAME_WIDTH = 170
MAX_FRAME_HEIGHT = 170
```

## 6. Tight crop lai frame

Bounding box sau connected component co the bi rong hon sprite that do buoc dilate.

Vi vay script crop lai lan nua tren anh goc:

1. Lay vung bounding box.
2. Tim lai pixel khac nen ben trong vung do.
3. Tao bounding box sat hon quanh sprite that.

Ket qua la frame duoc crop gon nhat co the ma van giu du effect.

## 7. Xuat frame PNG trong suot

Moi frame duoc xuat thanh file rieng:

```text
assets/ichigo_frames/ichigo_000.png
assets/ichigo_frames/ichigo_001.png
...

assets/naruto_frames/naruto_000.png
assets/naruto_frames/naruto_001.png
...
```

Cac file frame nay co nen trong suot.

Preview thi co nen tim de de nhin:

```text
preview_first_120.png
preview_last_120.png
```

## 8. Manifest

Script xuat them:

```text
manifest.csv
manifest.json
```

Moi frame co thong tin:

```json
{
  "file": "ichigo_000.png",
  "x": 12,
  "y": 34,
  "w": 68,
  "h": 45
}
```

Y nghia:

- `file`: ten frame sau khi cat.
- `x`, `y`: toa do frame trong sprite sheet goc.
- `w`, `h`: kich thuoc frame sau khi crop.

## 9. Sap xep frame

Frame duoc sap xep theo thu tu doc sprite sheet:

```text
tu trai sang phai
tu tren xuong duoi
```

Trong script:

```python
frames.sort(key=lambda item: (item["y"] // 24, item["x"]))
```

`item["y"] // 24` giup gom frame theo hang gan dung, tranh truong hop frame trong cung mot hang co y lech nhau vai pixel.

Luu y quan trong: khi tao moveset, thu tu frame trong state khong bat buoc la tang dan theo so. Neu state can thu tu dac biet, config phai giu nguyen thu tu do.

Vi du:

```json
"attack_light": [95, 92, 96, 99]
```

Khong duoc sort thanh `[92, 95, 96, 99]`.

## 10. Convert sang RGB565 cho STM32

Sau khi co PNG trong suot, moi frame duoc convert sang C:

```text
PNG transparent
-> RGB565
-> pixel transparent thanh SPRITE_COLOR_KEY_RGB565
-> xuat .c/.h
```

Color key dang dung:

```c
#define SPRITE_COLOR_KEY_RGB565  0xF81FU
```

Khi ve, renderer bo qua pixel co mau nay.

## 11. Cach tinh thoi gian chay het mot state

Mot state co:

- So frame: `frame_count`
- Thoi gian moi frame: `frame_duration_ms`

Thoi gian chay het mot vong state:

```text
state_time_ms = frame_count * frame_duration_ms
```

Vi du:

```json
"run": {
  "frame_duration_ms": 80,
  "frames": [4, 5, 6, 7, 8]
}
```

State `run` co 5 frame:

```text
5 * 80ms = 400ms
```

Nghia la chay het mot vong run mat 0.4 giay.

## 12. Bang uoc tinh moveset Ichigo hien tai

Theo `assets/ichigo_frames/ichigo_moveset.json` hien tai:

| State | So frame | Duration moi frame | Tong thoi gian |
|---|---:|---:|---:|
| idle | 4 | 140ms | 560ms |
| run | 5 | 80ms | 400ms |
| attack_light | 4 | 75ms | 300ms |
| block | 1 | 120ms | 120ms |
| skill | 8 | 70ms | 560ms |
| jump | 8 | 90ms | 720ms |
| hit | 3 | 90ms | 270ms |
| dead | 4 | 120ms | 480ms |

## 13. Goi y thoi gian cho gameplay

Gia tri nen dung de animation game trong man hinh STM32 nhin on:

| Loai state | Duration/frame goi y |
|---|---:|
| idle | 120-180ms |
| run | 60-90ms |
| jump | 80-110ms |
| attack thuong | 60-90ms |
| skill | 60-90ms |
| hit | 70-110ms |
| dead | 100-160ms |

Neu duration qua thap, animation co the nhanh va LCD SPI khong theo kip, de bi nhay.

Neu duration qua cao, nhan vat se bi cam giac cham va khung hinh roi rac.

## 14. Uoc tinh thoi gian khi them nhan vat moi

Voi mot sprite sheet co nen phang nhu Ichigo/Naruto:

1. Tach frame tu dong: vai giay.
2. Tao preview: vai giay.
3. Chon moveset/state: tuy nguoi xem sprite, thuong mat lau nhat.
4. Convert sang RGB565 C: vai giay.
5. Build/flash STM32: khoang 10-20 giay tuy size firmware.

Phan ton cong nhat khong phai cat frame, ma la chon dung frame cho tung state.

## 15. Luu y khi sprite sheet khac

Neu nen khong phai mau phang, script can doi cach tach nen:

- Nen gan phang: dung tolerance mau.
- Nen phuc tap: can tach theo alpha, palette, hoac cat thu cong/ban tu dong.
- Sprite dinh sat nhau: can giam hoac tang `DILATE_SIZE`.
- Effect qua xa nhan vat: co the bi tach thanh component rieng, can gom theo khoang cach lon hon.

Voi sheet co nen phang ro rang, pipeline hien tai la nhanh va on dinh.
