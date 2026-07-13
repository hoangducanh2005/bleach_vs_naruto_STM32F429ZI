import json
import argparse
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CONFIG = ROOT / "assets" / "vizard_ichigo_frames" / "vizard_ichigo_moveset.json"
OUT_H = ROOT / "Core" / "Inc" / "vizard_moveset.h"
OUT_C = ROOT / "Core" / "Src" / "vizard_moveset.c"

COLOR_KEY = 0xF81F

def rgb565(r, g, b):
    value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    if value == COLOR_KEY:
        return 0xF81E
    return value

def state_macro(name):
    return f"VIZARD_MOVE_{name.upper()}"

def state_symbol(name):
    return f"vizard_move_{name}"

def load_frame(source_dir, frame_number):
    path = source_dir / f"vizard_ichigo_{frame_number:03d}.png"
    image = Image.open(path).convert("RGBA")
    width, height = image.size
    pixels = []

    for r, g, b, a in image.getdata():
        if a < 128:
            pixels.append(COLOR_KEY)
        else:
            pixels.append(rgb565(r, g, b))

    return path.stem, width, height, pixels

def parse_args():
    parser = argparse.ArgumentParser(description="Convert Vizard Ichigo moveset frames to C arrays.")
    parser.add_argument("--config", type=Path, default=DEFAULT_CONFIG)
    return parser.parse_args()

def load_moveset(config_path):
    config = json.loads(config_path.read_text(encoding="utf-8"))
    source_dir = ROOT / config["source_dir"]
    states = []

    for state_name, state in config["states"].items():
        frames = []
        for frame_number in state["frames"]:
            stem, width, height, pixels = load_frame(source_dir, frame_number)
            frames.append(
                {
                    "stem": stem,
                    "number": frame_number,
                    "width": width,
                    "height": height,
                    "pivot_x": width // 2,
                    "pivot_y": height,
                    "pixels": pixels,
                }
            )

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
  * @file           : vizard_moveset.h
  * @brief          : Vizard Ichigo gameplay animation frames generated from moveset JSON.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __VIZARD_MOVESET_H
#define __VIZARD_MOVESET_H

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

    lines.append(f"  VIZARD_MOVE_STATE_COUNT = {len(states)}U\n")
    lines.append(
        """} VizardMoveState;

typedef struct
{
  const uint16_t *pixels;
  uint16_t width;
  uint16_t height;
  int16_t pivotX;
  int16_t pivotY;
  uint16_t durationMs;
} VizardMoveFrame;

typedef struct
{
  const VizardMoveFrame *frames;
  uint8_t frameCount;
  uint8_t loop;
  uint8_t holdLast;
  const uint16_t * const *casterEffectFrames;
  uint8_t casterEffectFrameCount;
  uint16_t casterEffectWidth;
  uint16_t casterEffectHeight;
  const uint16_t * const *projectileFrames;
  uint8_t projectileFrameCount;
  uint16_t projectileWidth;
  uint16_t projectileHeight;
} VizardMoveAnimation;

extern const VizardMoveAnimation vizard_move_animations[VIZARD_MOVE_STATE_COUNT];

#ifdef __cplusplus
}
#endif

#endif /* __VIZARD_MOVESET_H */
"""
    )

    OUT_H.write_text("".join(lines), encoding="ascii")

def write_source(states):
    lines = [
        """/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : vizard_moveset.c
  * @brief          : Vizard Ichigo gameplay animation frames generated from moveset JSON.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "vizard_moveset.h"

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

        lines.append(f"static const VizardMoveFrame {symbol}_frames[] = {{\n")
        for idx, frame in enumerate(state["frames"]):
            comma = "," if idx + 1 < len(state["frames"]) else ""
            lines.append(
                f"  {{ {symbol}_{idx}_pixels, {frame['width']}U, {frame['height']}U, "
                f"{frame['pivot_x']}, {frame['pivot_y']}, {state['duration_ms']}U }}{comma}\n"
            )
        lines.append("};\n\n")

    lines.append("const VizardMoveAnimation vizard_move_animations[VIZARD_MOVE_STATE_COUNT] = {\n")

    for idx, state in enumerate(states):
        comma = "," if idx + 1 < len(states) else ""
        lines.append(
            f"  [{state['macro']}] = {{ {state['symbol']}_frames, "
            f"{len(state['frames'])}U, {state['loop']}U, {state['hold_last']}U, "
            f"0, 0U, 0U, 0U, 0, 0U, 0U, 0U }}{comma}\n"
        )

    lines.append("};\n")

    OUT_C.write_text("".join(lines), encoding="ascii")

def main():
    args = parse_args()
    states = load_moveset(args.config)
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
