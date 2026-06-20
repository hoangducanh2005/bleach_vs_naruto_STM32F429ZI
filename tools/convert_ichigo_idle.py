from pathlib import Path
import re

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
INPUT_DIR = ROOT / "assets" / "raw" / "ichigo"
OUT_H = ROOT / "Core" / "Inc" / "sprite_data.h"
OUT_C = ROOT / "Core" / "Src" / "sprite_data.c"

COLOR_KEY = 0xF81F
BACKGROUND_TOLERANCE = 6


def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


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


def frame_sort_key(path):
    match = re.search(r"(\d+)", path.stem)
    if match:
        return int(match.group(1))

    return path.stem


def load_frames():
    files = sorted(INPUT_DIR.glob("*.png"), key=frame_sort_key)

    if len(files) != 4:
        raise ValueError(f"Expected 4 Ichigo idle PNG frames, found {len(files)}")

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
        raise ValueError("All idle frames must have the same size")

    return frames


def write_header(width, height):
    OUT_H.write_text(
        f"""/* USER CODE BEGIN Header */
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
extern "C" {{
#endif

#include <stdint.h>

#define SPRITE_COLOR_KEY_RGB565  0xF81FU
#define ICHIGO_IDLE_FRAME_COUNT  4U
#define ICHIGO_IDLE_WIDTH        {width}U
#define ICHIGO_IDLE_HEIGHT       {height}U

extern const uint16_t ichigo_idle_0[ICHIGO_IDLE_WIDTH * ICHIGO_IDLE_HEIGHT];
extern const uint16_t ichigo_idle_1[ICHIGO_IDLE_WIDTH * ICHIGO_IDLE_HEIGHT];
extern const uint16_t ichigo_idle_2[ICHIGO_IDLE_WIDTH * ICHIGO_IDLE_HEIGHT];
extern const uint16_t ichigo_idle_3[ICHIGO_IDLE_WIDTH * ICHIGO_IDLE_HEIGHT];
extern const uint16_t * const ichigo_idle_frames[ICHIGO_IDLE_FRAME_COUNT];

#ifdef __cplusplus
}}
#endif

#endif /* __SPRITE_DATA_H */
""",
        encoding="ascii",
    )


def write_source(frames):
    parts = [
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

    for idx, (_, _, _, pixels) in enumerate(frames):
        parts.append(
            f"const uint16_t ichigo_idle_{idx}[ICHIGO_IDLE_WIDTH * ICHIGO_IDLE_HEIGHT] = {{\n"
        )

        for i in range(0, len(pixels), 12):
            row = ", ".join(f"0x{value:04X}U" for value in pixels[i : i + 12])
            comma = "," if i + 12 < len(pixels) else ""
            parts.append(f"  {row}{comma}\n")

        parts.append("};\n\n")

    parts.append("const uint16_t * const ichigo_idle_frames[ICHIGO_IDLE_FRAME_COUNT] = {\n")

    for i in range(len(frames)):
        comma = "," if i + 1 < len(frames) else ""
        parts.append(f"  ichigo_idle_{i}{comma}\n")

    parts.append("};\n")
    OUT_C.write_text("".join(parts), encoding="ascii")


def main():
    frames = load_frames()
    _, width, height, _ = frames[0]

    write_header(width, height)
    write_source(frames)

    transparent_pixels = sum(
        1 for _, _, _, pixels in frames for value in pixels if value == COLOR_KEY
    )
    total_pixels = sum(len(pixels) for _, _, _, pixels in frames)
    print(
        f"Generated {OUT_H.relative_to(ROOT)} and {OUT_C.relative_to(ROOT)} "
        f"({width}x{height}, {len(frames)} frames, "
        f"{transparent_pixels}/{total_pixels} transparent pixels)"
    )


if __name__ == "__main__":
    main()
