import json
from collections import deque
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
CONFIG = ROOT / "assets" / "naruto_frames" / "naruto_moveset.json"
OUT_H = ROOT / "Core" / "Inc" / "naruto_moveset.h"
OUT_C = ROOT / "Core" / "Src" / "naruto_moveset.c"

COLOR_KEY = 0xF81F


def rgb565(r, g, b):
    value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    if value == COLOR_KEY:
        return 0xF81E
    return value


def state_macro(name):
    return f"NARUTO_MOVE_{name.upper()}"


def state_symbol(name):
    return f"naruto_move_{name}"


def foreground_mask(image, background):
    pixels = image.load()
    width, height = image.size
    mask = bytearray(width * height)

    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            if a < 128:
                continue

            if abs(r - background[0]) + abs(g - background[1]) + abs(b - background[2]) > 18:
                mask[(y * width) + x] = 1

    return mask


def find_components(mask, width, height):
    visited = bytearray(width * height)
    components = []

    for y in range(height):
        for x in range(width):
            start = (y * width) + x
            if visited[start] or mask[start] == 0:
                continue

            queue = deque([(x, y)])
            visited[start] = 1
            min_x = max_x = x
            min_y = max_y = y
            count = 0

            while queue:
                cx, cy = queue.popleft()
                count += 1
                min_x = min(min_x, cx)
                max_x = max(max_x, cx)
                min_y = min(min_y, cy)
                max_y = max(max_y, cy)

                for ny in range(cy - 1, cy + 2):
                    if ny < 0 or ny >= height:
                        continue
                    for nx in range(cx - 1, cx + 2):
                        if nx < 0 or nx >= width:
                            continue
                        idx = (ny * width) + nx
                        if visited[idx] or mask[idx] == 0:
                            continue
                        visited[idx] = 1
                        queue.append((nx, ny))

            components.append((min_x, min_y, max_x + 1, max_y + 1, count))

    return components


def transparent_from_background(image):
    image = image.convert("RGBA")
    background = image.getpixel((0, 0))[:3]
    pixels = image.load()
    width, height = image.size

    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            if a < 128:
                pixels[x, y] = (0, 0, 0, 0)
            elif abs(r - background[0]) + abs(g - background[1]) + abs(b - background[2]) <= 18:
                pixels[x, y] = (0, 0, 0, 0)

    return image


def load_effect_frames(effect_config):
    sheet = Image.open(ROOT / effect_config["sheet"]).convert("RGBA")
    background = sheet.getpixel((0, 0))[:3]
    mask = foreground_mask(sheet, background)
    components = find_components(mask, sheet.width, sheet.height)
    components = [
        component
        for component in components
        if component[4] > 400 and (component[2] - component[0]) > 20 and (component[3] - component[1]) > 20
    ]
    components.sort(key=lambda item: item[0])

    frames = []
    for left, top, right, bottom, _ in components[:3]:
        frame = transparent_from_background(sheet.crop((left, top, right, bottom)))
        target_width = int(effect_config["target_width"])
        target_height = max(1, round(frame.height * (target_width / frame.width)))
        frame = frame.resize((target_width, target_height), Image.Resampling.LANCZOS)
        frames.append(frame)

    if len(frames) != 3:
        raise RuntimeError(f"Expected 3 Rasengan frames, got {len(frames)}")

    return frames


def attach_effect(frame, frame_number, effect_config, effect_frames):
    if frame_number < int(effect_config["first_source_frame"]):
        return frame, 0, 0

    effect_index = (frame_number - int(effect_config["first_source_frame"])) % len(effect_frames)
    effect = effect_frames[effect_index]
    effect_x = frame.width + int(effect_config["offset_x"])
    effect_y = int(effect_config["offset_y"])

    left_pad = max(0, -effect_x)
    top_pad = max(0, -effect_y)
    right = max(frame.width + left_pad, effect_x + left_pad + effect.width)
    bottom = max(frame.height + top_pad, effect_y + top_pad + effect.height)

    composite = Image.new("RGBA", (right, bottom), (0, 0, 0, 0))
    composite.alpha_composite(frame, (left_pad, top_pad))
    composite.alpha_composite(effect, (effect_x + left_pad, effect_y + top_pad))

    return composite, left_pad, top_pad


def image_to_pixels(image):
    pixels = []
    for r, g, b, a in image.getdata():
        if a < 128:
            pixels.append(COLOR_KEY)
        else:
            pixels.append(rgb565(r, g, b))
    return pixels


def load_frame(source_dir, frame_number, state):
    path = source_dir / f"naruto_{frame_number:03d}.png"
    image = Image.open(path).convert("RGBA")
    original_width, original_height = image.size
    left_pad = top_pad = 0

    effect_config = state.get("attached_effect")
    if effect_config is not None:
        effect_frames = state["_effect_frames"]
        image, left_pad, top_pad = attach_effect(image, frame_number, effect_config, effect_frames)

    return {
        "stem": path.stem,
        "number": frame_number,
        "width": image.width,
        "height": image.height,
        "pivot_x": (original_width // 2) + left_pad,
        "pivot_y": original_height + top_pad,
        "pixels": image_to_pixels(image),
    }


def load_moveset():
    config = json.loads(CONFIG.read_text(encoding="utf-8"))
    source_dir = ROOT / config["source_dir"]
    states = []

    for state_name, state_config in config["states"].items():
        state = dict(state_config)
        if "attached_effect" in state:
            state["_effect_frames"] = load_effect_frames(state["attached_effect"])

        frames = [load_frame(source_dir, frame_number, state) for frame_number in state["frames"]]
        states.append(
            {
                "name": state_name,
                "macro": state_macro(state_name),
                "symbol": state_symbol(state_name),
                "loop": 1 if state.get("loop", False) else 0,
                "hold_last": 1 if state.get("hold_last", False) else 0,
                "duration_ms": int(state.get("frame_duration_ms", 90)),
                "frames": frames,
            }
        )

    return states


def write_header(states):
    lines = [
        """/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : naruto_moveset.h
  * @brief          : Naruto gameplay animation frames generated from moveset JSON.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __NARUTO_MOVESET_H
#define __NARUTO_MOVESET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
"""
    ]

    for idx, state in enumerate(states):
        lines.append(f"  {state['macro']} = {idx}U,\n")

    lines.append(f"  NARUTO_MOVE_STATE_COUNT = {len(states)}U\n")
    lines.append(
        """} NarutoMoveState;

typedef struct
{
  const uint16_t *pixels;
  uint16_t width;
  uint16_t height;
  int16_t pivotX;
  int16_t pivotY;
  uint16_t durationMs;
} NarutoMoveFrame;

typedef struct
{
  const NarutoMoveFrame *frames;
  uint8_t frameCount;
  uint8_t loop;
  uint8_t holdLast;
} NarutoMoveAnimation;

extern const NarutoMoveAnimation naruto_move_animations[NARUTO_MOVE_STATE_COUNT];

#ifdef __cplusplus
}
#endif

#endif /* __NARUTO_MOVESET_H */
"""
    )

    OUT_H.write_text("".join(lines), encoding="ascii")


def write_source(states):
    lines = [
        """/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : naruto_moveset.c
  * @brief          : Naruto gameplay animation frames generated from moveset JSON.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "naruto_moveset.h"

#include "sprite_data.h"

"""
    ]

    for state in states:
        symbol = state["symbol"]

        for idx, frame in enumerate(state["frames"]):
            lines.append(
                f"static const uint16_t {symbol}_{idx}_pixels"
                f"[{frame['width']}U * {frame['height']}U] = {{\n"
            )

            pixels = frame["pixels"]
            for i in range(0, len(pixels), 12):
                row = ", ".join(f"0x{value:04X}U" for value in pixels[i : i + 12])
                comma = "," if i + 12 < len(pixels) else ""
                lines.append(f"  {row}{comma}\n")

            lines.append("};\n\n")

        lines.append(f"static const NarutoMoveFrame {symbol}_frames[] = {{\n")
        for idx, frame in enumerate(state["frames"]):
            comma = "," if idx + 1 < len(state["frames"]) else ""
            lines.append(
                f"  {{ {symbol}_{idx}_pixels, {frame['width']}U, {frame['height']}U, "
                f"{frame['pivot_x']}, {frame['pivot_y']}, {state['duration_ms']}U }}{comma}\n"
            )
        lines.append("};\n\n")

    lines.append("const NarutoMoveAnimation naruto_move_animations[NARUTO_MOVE_STATE_COUNT] = {\n")

    for idx, state in enumerate(states):
        comma = "," if idx + 1 < len(states) else ""
        lines.append(
            f"  [{state['macro']}] = {{ {state['symbol']}_frames, "
            f"{len(state['frames'])}U, {state['loop']}U, {state['hold_last']}U }}{comma}\n"
        )

    lines.append("};\n")

    OUT_C.write_text("".join(lines), encoding="ascii")


def main():
    states = load_moveset()
    write_header(states)
    write_source(states)

    total_frames = sum(len(state["frames"]) for state in states)
    total_pixels = sum(len(frame["pixels"]) for state in states for frame in state["frames"])
    transparent_pixels = sum(
        1
        for state in states
        for frame in state["frames"]
        for pixel in frame["pixels"]
        if pixel == COLOR_KEY
    )

    print(f"Generated {OUT_H.relative_to(ROOT)}")
    print(f"Generated {OUT_C.relative_to(ROOT)}")
    print(f"{len(states)} states, {total_frames} frames")
    print(f"{transparent_pixels}/{total_pixels} transparent pixels")

    for state in states:
        sizes = sorted({(frame["width"], frame["height"]) for frame in state["frames"]})
        print(f"{state['name']}: {len(state['frames'])} frames, sizes={sizes}")


if __name__ == "__main__":
    main()
