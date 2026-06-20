from pathlib import Path
import re

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
INPUT_ROOT = ROOT / "assets" / "raw"
OUT_H = ROOT / "Core" / "Inc" / "sprite_data.h"
OUT_C = ROOT / "Core" / "Src" / "sprite_data.c"

COLOR_KEY = 0xF81F
BACKGROUND_TOLERANCE = 6

ASSET_GROUPS = [
    ("ichigo", r"ichiidle.*\.png$", "ICHIGO_IDLE", "ichigo_idle", None),
    ("ichigo", r"ichigetsuga.*\.png$", "ICHIGO_GETSUGA", "ichigo_getsuga", {2, 5, 6, 8, 9}),
    ("ichigo_projectiles", r"getsuga.*\.png$", "GETSUGA_PROJECTILE", "getsuga_projectile", None),
    ("naruto", r"naruto.*\.png$", "NARUTO_IDLE", "naruto_idle", None),
]


def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def frame_sort_key(path):
    match = re.search(r"(\d+)", path.stem)
    if match:
        return int(match.group(1))

    return path.stem


def is_background(pixel, background):
    r, g, b, a = pixel
    br, bg, bb, ba = background

    if a < 128:
        return True

    if ba < 128:
        return False

    return (
        abs(r - br) <= BACKGROUND_TOLERANCE
        and abs(g - bg) <= BACKGROUND_TOLERANCE
        and abs(b - bb) <= BACKGROUND_TOLERANCE
    )


def frame_number(path):
    match = re.search(r"(\d+)", path.stem)
    if match:
        return int(match.group(1))

    return None


def load_frames(folder_name, pattern, selected_numbers):
    folder = INPUT_ROOT / folder_name
    regex = re.compile(pattern, re.IGNORECASE)
    files = sorted((path for path in folder.glob("*.png") if regex.match(path.name)), key=frame_sort_key)

    if selected_numbers is not None:
        files = [path for path in files if frame_number(path) in selected_numbers]

    if not files:
        raise ValueError(f"No PNG frames matching {pattern} found in {folder}")

    frames = []

    for file_path in files:
        image = Image.open(file_path).convert("RGBA")
        width, height = image.size
        background = image.getpixel((0, 0))
        pixels = []

        for pixel in image.getdata():
            if is_background(pixel, background):
                pixels.append(COLOR_KEY)
            else:
                r, g, b, _ = pixel
                pixels.append(rgb565(r, g, b))

        frames.append((file_path.stem, width, height, pixels))

    if len({(width, height) for _, width, height, _ in frames}) != 1:
        raise ValueError(f"All frames matching {pattern} in {folder} must have the same size")

    return frames


def write_header(all_data):
    lines = [
        """/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : sprite_data.h
  * @brief          : RGB565 sprite data generated from PNG assets.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __SPRITE_DATA_H
#define __SPRITE_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define SPRITE_COLOR_KEY_RGB565  0xF81FU

"""
    ]

    for _, macro_prefix, symbol_prefix, frames in all_data:
        _, width, height, _ = frames[0]
        lines.append(f"#define {macro_prefix}_FRAME_COUNT  {len(frames)}U\n")
        lines.append(f"#define {macro_prefix}_WIDTH        {width}U\n")
        lines.append(f"#define {macro_prefix}_HEIGHT       {height}U\n\n")

        for idx in range(len(frames)):
            lines.append(
                f"extern const uint16_t {symbol_prefix}_{idx}"
                f"[{macro_prefix}_WIDTH * {macro_prefix}_HEIGHT];\n"
            )

        lines.append(
            f"extern const uint16_t * const {symbol_prefix}_frames"
            f"[{macro_prefix}_FRAME_COUNT];\n\n"
        )

    lines.append(
        """#ifdef __cplusplus
}
#endif

#endif /* __SPRITE_DATA_H */
"""
    )

    OUT_H.write_text("".join(lines), encoding="ascii")


def write_source(all_data):
    lines = [
        """/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : sprite_data.c
  * @brief          : RGB565 sprite data generated from PNG assets.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "sprite_data.h"

"""
    ]

    for _, macro_prefix, symbol_prefix, frames in all_data:
        for idx, (_, _, _, pixels) in enumerate(frames):
            lines.append(
                f"const uint16_t {symbol_prefix}_{idx}"
                f"[{macro_prefix}_WIDTH * {macro_prefix}_HEIGHT] = {{\n"
            )

            for i in range(0, len(pixels), 12):
                row = ", ".join(f"0x{value:04X}U" for value in pixels[i : i + 12])
                comma = "," if i + 12 < len(pixels) else ""
                lines.append(f"  {row}{comma}\n")

            lines.append("};\n\n")

        lines.append(
            f"const uint16_t * const {symbol_prefix}_frames"
            f"[{macro_prefix}_FRAME_COUNT] = {{\n"
        )

        for idx in range(len(frames)):
            comma = "," if idx + 1 < len(frames) else ""
            lines.append(f"  {symbol_prefix}_{idx}{comma}\n")

        lines.append("};\n\n")

    OUT_C.write_text("".join(lines), encoding="ascii")


def main():
    all_data = []

    for folder_name, pattern, macro_prefix, symbol_prefix, selected_numbers in ASSET_GROUPS:
        frames = load_frames(folder_name, pattern, selected_numbers)
        all_data.append((symbol_prefix, macro_prefix, symbol_prefix, frames))

    write_header(all_data)
    write_source(all_data)

    for group_name, _, _, frames in all_data:
        _, width, height, pixels = frames[0]
        transparent_pixels = sum(
            1 for _, _, _, frame_pixels in frames for value in frame_pixels if value == COLOR_KEY
        )
        total_pixels = sum(len(frame_pixels) for _, _, _, frame_pixels in frames)
        print(
            f"{group_name}: {width}x{height}, {len(frames)} frames, "
            f"{transparent_pixels}/{total_pixels} transparent pixels"
        )

    print(f"Generated {OUT_H.relative_to(ROOT)} and {OUT_C.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
