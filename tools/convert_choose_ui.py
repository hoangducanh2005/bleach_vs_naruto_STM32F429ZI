from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
ASSET_ROOT = ROOT / "assets" / "choose_UI"
OUT_H = ROOT / "Core" / "Inc" / "choose_ui_assets.h"
OUT_C = ROOT / "Core" / "Src" / "choose_ui_assets.c"

CHARACTERS = [
    ("NARUTO", "naruto", "avatar/naruto.png", "banner/narruto.jpg"),
    ("NINE_TAIL", "nine_tail", "avatar/nine_tail.png", "banner/narrutoninetail.jpg"),
    ("SASUKE", "sasuke", "avatar/sasuke.jpg", "banner/sasuke.jpg"),
    ("ICHIGO", "ichigo", "avatar/ichigo.png", "banner/ichigo.jpg"),
    ("HOLLOW", "hollow", "avatar/ichigo_hollowmask.png", "banner/ichigo_hollow.jpg"),
    ("RUKIA", "rukia", "avatar/rukia.jpg", "banner/rukia.jpg"),
]

AVATAR_SIZE = (40, 40)
BANNER_SIZE = (150, 50)


def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def fit_cover(image, size):
    image = image.convert("RGB")
    src_w, src_h = image.size
    dst_w, dst_h = size
    scale = max(dst_w / src_w, dst_h / src_h)
    scaled = image.resize((round(src_w * scale), round(src_h * scale)), Image.Resampling.LANCZOS)
    left = (scaled.width - dst_w) // 2
    top = (scaled.height - dst_h) // 2
    return scaled.crop((left, top, left + dst_w, top + dst_h))


def image_to_rgb565_bytes(path, size):
    image = fit_cover(Image.open(path), size)
    data = []

    for r, g, b in image.getdata():
        color = rgb565(r, g, b)
        data.append(color & 0xFF)
        data.append(color >> 8)

    return data


def write_array(lines, symbol, data):
    lines.append(f"const uint8_t {symbol}[] = {{\n")

    for i in range(0, len(data), 16):
        row = ", ".join(f"0x{value:02X}" for value in data[i : i + 16])
        comma = "," if i + 16 < len(data) else ""
        lines.append(f"  {row}{comma}\n")

    lines.append("};\n\n")


def main():
    header = [
        """#ifndef CHOOSE_UI_ASSETS_H
#define CHOOSE_UI_ASSETS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define CHOOSE_CHARACTER_COUNT 6U
#define CHOOSE_AVATAR_WIDTH    40U
#define CHOOSE_AVATAR_HEIGHT   40U
#define CHOOSE_AVATAR_SIZE     3200U
#define CHOOSE_BANNER_WIDTH    150U
#define CHOOSE_BANNER_HEIGHT   50U
#define CHOOSE_BANNER_SIZE     15000U

typedef enum
{
"""
    ]

    for index, (enum_name, _, _, _) in enumerate(CHARACTERS):
        comma = "," if index + 1 < len(CHARACTERS) else ""
        header.append(f"  CHOOSE_CHARACTER_{enum_name} = {index}U{comma}\n")

    header.append(
        """} ChooseCharacterId;

"""
    )

    source = [
        """#include "choose_ui_assets.h"

"""
    ]

    avatar_symbols = []
    banner_symbols = []

    for _, symbol_name, avatar_path, banner_path in CHARACTERS:
        avatar_symbol = f"choose_avatar_{symbol_name}_map"
        banner_symbol = f"choose_banner_{symbol_name}_map"
        avatar_symbols.append(avatar_symbol)
        banner_symbols.append(banner_symbol)

        header.append(f"extern const uint8_t {avatar_symbol}[CHOOSE_AVATAR_SIZE];\n")
        header.append(f"extern const uint8_t {banner_symbol}[CHOOSE_BANNER_SIZE];\n")

        write_array(source, avatar_symbol, image_to_rgb565_bytes(ASSET_ROOT / avatar_path, AVATAR_SIZE))
        write_array(source, banner_symbol, image_to_rgb565_bytes(ASSET_ROOT / banner_path, BANNER_SIZE))

    header.append(
        """
extern const uint8_t * const choose_avatar_maps[CHOOSE_CHARACTER_COUNT];
extern const uint8_t * const choose_banner_maps[CHOOSE_CHARACTER_COUNT];

#ifdef __cplusplus
}
#endif

#endif /* CHOOSE_UI_ASSETS_H */
"""
    )

    source.append("const uint8_t * const choose_avatar_maps[CHOOSE_CHARACTER_COUNT] = {\n")
    for index, symbol in enumerate(avatar_symbols):
        comma = "," if index + 1 < len(avatar_symbols) else ""
        source.append(f"  {symbol}{comma}\n")
    source.append("};\n\n")

    source.append("const uint8_t * const choose_banner_maps[CHOOSE_CHARACTER_COUNT] = {\n")
    for index, symbol in enumerate(banner_symbols):
        comma = "," if index + 1 < len(banner_symbols) else ""
        source.append(f"  {symbol}{comma}\n")
    source.append("};\n")

    OUT_H.write_text("".join(header), encoding="ascii")
    OUT_C.write_text("".join(source), encoding="ascii")

    print(f"Generated {OUT_H.relative_to(ROOT)} and {OUT_C.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
