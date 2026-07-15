from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
FRAME_DIR = ROOT / "assets" / "naruto_direction" / "frames"
OUT_H = ROOT / "Core" / "Inc" / "naruto_direction_moveset.h"
OUT_C = ROOT / "Core" / "Src" / "naruto_direction_moveset.c"
COLOR_KEY = 0xF81F
FRAME_DURATION_MS = 60
TARGET_HEIGHT = 44


def rgb565(r, g, b):
    value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    return 0xF81E if value == COLOR_KEY else value


def load_frame(path):
    image = Image.open(path).convert("RGBA")
    if image.height != TARGET_HEIGHT:
        width = max(1, round(image.width * TARGET_HEIGHT / image.height))
        image = image.resize((width, TARGET_HEIGHT), Image.Resampling.NEAREST)

    pixels = []
    for r, g, b, a in image.getdata():
        if a < 128:
            pixels.append(COLOR_KEY)
        else:
            pixels.append(rgb565(r, g, b))

    return {
        "path": path,
        "width": image.width,
        "height": image.height,
        "pivot_x": image.width // 2,
        "pivot_y": image.height,
        "pixels": pixels,
    }


def encode_rle(pixels):
    if not pixels:
        return []

    encoded = []
    current = pixels[0]
    count = 1

    for pixel in pixels[1:]:
        if pixel == current and count < 0xFFFF:
            count += 1
        else:
            encoded.extend((count, current))
            current = pixel
            count = 1

    encoded.extend((count, current))
    return encoded


def write_header(frames):
    lines = [
        """#ifndef __NARUTO_DIRECTION_MOVESET_H
#define __NARUTO_DIRECTION_MOVESET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define NARUTO_DIRECTION_FRAME_COUNT """
        f"{len(frames)}U\n\n"
        """typedef struct
{
  const uint16_t *pixels;
  uint16_t width;
  uint16_t height;
  int16_t pivotX;
  int16_t pivotY;
  uint16_t durationMs;
} NarutoDirectionFrame;

extern const NarutoDirectionFrame naruto_direction_frames[NARUTO_DIRECTION_FRAME_COUNT];

#ifdef __cplusplus
}
#endif

#endif /* __NARUTO_DIRECTION_MOVESET_H */
"""
    ]
    OUT_H.write_text("".join(lines), encoding="ascii")


def write_source(frames):
    lines = [
        """#include "naruto_direction_moveset.h"

"""
    ]

    for index, frame in enumerate(frames):
        encoded = encode_rle(frame["pixels"])
        frame["rle"] = encoded
        lines.append(
            f"static const uint16_t naruto_direction_{index:03d}_pixels"
            f"[{len(encoded)}U] = {{\n"
        )
        for i in range(0, len(encoded), 12):
            row = ", ".join(f"0x{value:04X}U" for value in encoded[i : i + 12])
            comma = "," if i + 12 < len(encoded) else ""
            lines.append(f"  {row}{comma}\n")
        lines.append("};\n\n")

    lines.append(
        "const NarutoDirectionFrame naruto_direction_frames[NARUTO_DIRECTION_FRAME_COUNT] = {\n"
    )
    for index, frame in enumerate(frames):
        comma = "," if index + 1 < len(frames) else ""
        lines.append(
            f"  {{ naruto_direction_{index:03d}_pixels, {frame['width']}U, "
            f"{frame['height']}U, {frame['pivot_x']}, {frame['pivot_y']}, "
            f"{FRAME_DURATION_MS}U }}{comma}\n"
        )
    lines.append("};\n")

    OUT_C.write_text("".join(lines), encoding="ascii")


def main():
    paths = sorted(FRAME_DIR.glob("naruto_direction_*.png"))
    frames = [load_frame(path) for path in paths]
    write_header(frames)
    write_source(frames)
    print(f"Generated {OUT_H.relative_to(ROOT)}")
    print(f"Generated {OUT_C.relative_to(ROOT)}")
    print(f"{len(frames)} frames")
    raw_words = sum(frame["width"] * frame["height"] for frame in frames)
    rle_words = sum(len(frame["rle"]) for frame in frames)
    print(f"RLE words: {rle_words}/{raw_words}")


if __name__ == "__main__":
    main()
