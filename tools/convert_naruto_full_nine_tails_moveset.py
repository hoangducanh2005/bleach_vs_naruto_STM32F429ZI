import json
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
CONFIG = ROOT / "assets" / "raw" / "naruto_full_nine_tails_frames" / "naruto_full_nine_tails_moveset.json"
OUT_H = ROOT / "Core" / "Inc" / "naruto_full_nine_tails_moveset.h"
OUT_C = ROOT / "Core" / "Src" / "naruto_full_nine_tails_moveset.c"

COLOR_KEY = 0xF81F


def rgb565(r, g, b):
    value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    if value == COLOR_KEY:
        return 0xF81E
    return value


def state_macro(name):
    return f"NARUTO_FULL_NINE_TAILS_MOVE_{name.upper()}"


def state_symbol(name):
    return f"naruto_full_nine_tails_move_{name}"


def load_frame(frames_root, strip_idx, frame_idx):
    """Load a single frame from its strip subdirectory."""
    subdir = frames_root / f"naruto_full_nine_tails_{strip_idx}"
    path = subdir / f"naruto_full_nine_tails_{strip_idx}_{frame_idx:03d}.png"
    if not path.exists():
        raise FileNotFoundError(f"Frame not found: {path}")
    image = Image.open(path).convert("RGBA")
    width, height = image.size
    pixels = []
    for r, g, b, a in image.getdata():
        if a < 128:
            pixels.append(COLOR_KEY)
        else:
            pixels.append(rgb565(r, g, b))
    return path.stem, width, height, pixels


def load_frame_refs(frames_root, frame_refs):
    """Load a list of frame references [{strip, frame}, ...].
    Pads all frames to the same (max_w, max_h) so the C struct can use uniform dimensions."""
    raw = []
    for ref in frame_refs:
        stem, w, h, pixels = load_frame(frames_root, ref["strip"], ref["frame"])
        raw.append({"stem": stem, "strip": ref["strip"], "frame_idx": ref["frame"],
                    "width": w, "height": h, "pixels": pixels})

    if not raw:
        return [], 0, 0

    max_w = max(f["width"] for f in raw)
    max_h = max(f["height"] for f in raw)

    # Pad frames that are smaller than max_w x max_h (fill with COLOR_KEY = transparent)
    for f in raw:
        if f["width"] != max_w or f["height"] != max_h:
            padded = []
            orig = f["pixels"]
            ow, oh = f["width"], f["height"]
            for y in range(max_h):
                for x in range(max_w):
                    if x < ow and y < oh:
                        padded.append(orig[y * ow + x])
                    else:
                        padded.append(COLOR_KEY)
            f["pixels"] = padded
            f["width"] = max_w
            f["height"] = max_h

    return raw, max_w, max_h


def load_moveset():
    config = json.loads(CONFIG.read_text(encoding="utf-8"))
    frames_root = ROOT / config["source_dir"]
    states = []

    # Find the idle state's pivot_x to align skill frames' left edges
    idle_ref = config["states"]["idle"]["frames"][0]
    _, idle_w, _, _ = load_frame(frames_root, idle_ref["strip"], idle_ref["frame"])
    idle_pivot_x = idle_w // 2

    for state_name, state in config["states"].items():
        # Load main character frames
        char_frames = []
        for ref in state["frames"]:
            stem, w, h, pixels = load_frame(frames_root, ref["strip"], ref["frame"])
            
            # Align skill1 left edge to idle left edge by matching pivot_x
            p_x = idle_pivot_x if state_name == "skill1" else (w // 2)
            
            char_frames.append({
                "stem": stem, "strip": ref["strip"], "frame_idx": ref["frame"],
                "width": w, "height": h,
                "pivot_x": p_x, "pivot_y": h,
                "pixels": pixels,
            })

        # Load caster_effect frames (optional)
        caster_data = state.get("caster_effect")
        caster_frames, caster_w, caster_h = [], 0, 0
        caster_start = 0
        caster_loop = False
        if caster_data:
            caster_frames, caster_w, caster_h = load_frame_refs(frames_root, caster_data["frames"])
            caster_start = caster_data.get("start_frame_idx", 0)
            caster_loop = caster_data.get("loop", False)

        # Load projectile frames (optional)
        proj_data = state.get("projectile")
        proj_frames, proj_w, proj_h = [], 0, 0
        proj_loop = False
        if proj_data:
            proj_frames, proj_w, proj_h = load_frame_refs(frames_root, proj_data["frames"])
            proj_loop = proj_data.get("loop", False)

        states.append({
            "name": state_name,
            "macro": state_macro(state_name),
            "symbol": state_symbol(state_name),
            "loop": 1 if state.get("loop", False) else 0,
            "hold_last": 1 if state.get("hold_last", False) else 0,
            "duration_ms": int(state.get("frame_duration_ms", 90)),
            "frames": char_frames,
            "caster_frames": caster_frames,
            "caster_w": caster_w,
            "caster_h": caster_h,
            "caster_start": caster_start,
            "caster_loop": 1 if caster_loop else 0,
            "proj_frames": proj_frames,
            "proj_w": proj_w,
            "proj_h": proj_h,
            "proj_loop": 1 if proj_loop else 0,
        })

    return states


def write_header(states):
    lines = [
        """/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : naruto_full_nine_tails_moveset.h
  * @brief          : Naruto Full Nine Tails gameplay animation frames generated from moveset JSON.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __NARUTO_FULL_NINE_TAILS_MOVESET_H
#define __NARUTO_FULL_NINE_TAILS_MOVESET_H

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

    lines.append(f"  NARUTO_FULL_NINE_TAILS_MOVE_STATE_COUNT = {len(states)}U\n")
    lines.append(
        """} NarutoFullNineTailsMoveState;

typedef struct
{
  const uint16_t *pixels;
  uint16_t width;
  uint16_t height;
  int16_t pivotX;
  int16_t pivotY;
  uint16_t durationMs;
} NarutoFullNineTailsMoveFrame;

typedef struct
{
  const NarutoFullNineTailsMoveFrame *frames;
  uint8_t frameCount;
  uint8_t loop;
  uint8_t holdLast;
  const uint16_t * const *casterEffectFrames;
  uint8_t casterEffectFrameCount;
  uint8_t casterEffectStartFrameIdx;
  uint8_t casterEffectLoop;
  uint16_t casterEffectWidth;
  uint16_t casterEffectHeight;
  const uint16_t * const *projectileFrames;
  uint8_t projectileFrameCount;
  uint8_t projectileLoop;
  uint16_t projectileWidth;
  uint16_t projectileHeight;
} NarutoFullNineTailsMoveAnimation;

extern const NarutoFullNineTailsMoveAnimation naruto_full_nine_tails_move_animations[NARUTO_FULL_NINE_TAILS_MOVE_STATE_COUNT];

#ifdef __cplusplus
}
#endif

#endif /* __NARUTO_FULL_NINE_TAILS_MOVESET_H */
"""
    )

    OUT_H.write_text("".join(lines), encoding="ascii")


def emit_pixel_array(lines, var_name, frame, w, h):
    """Emit a static uint16_t pixel array."""
    lines.append(f"static const uint16_t {var_name}[{w}U * {h}U] = {{\n")
    pixels = frame["pixels"]
    for i in range(0, len(pixels), 12):
        row = ", ".join(f"0x{v:04X}U" for v in pixels[i: i + 12])
        comma = "," if i + 12 < len(pixels) else ""
        lines.append(f"  {row}{comma}\n")
    lines.append("};\n\n")


def write_source(states):
    lines = [
        """/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : naruto_full_nine_tails_moveset.c
  * @brief          : Naruto Full Nine Tails gameplay animation frames generated from moveset JSON.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "naruto_full_nine_tails_moveset.h"

#include "sprite_data.h"

"""
    ]

    for state in states:
        symbol = state["symbol"]

        # --- Character frames ---
        for idx, frame in enumerate(state["frames"]):
            emit_pixel_array(lines, f"{symbol}_{idx}_pixels", frame, frame["width"], frame["height"])

        if state["frames"]:
            lines.append(f"static const NarutoFullNineTailsMoveFrame {symbol}_frames[] = {{\n")
            for idx, frame in enumerate(state["frames"]):
                comma = "," if idx + 1 < len(state["frames"]) else ""
                lines.append(
                    f"  {{ {symbol}_{idx}_pixels, {frame['width']}U, {frame['height']}U, "
                    f"{frame['pivot_x']}, {frame['pivot_y']}, {state['duration_ms']}U }}{comma}\n"
                )
            lines.append("};\n\n")
        else:
            lines.append(f"static const NarutoFullNineTailsMoveFrame *{symbol}_frames = 0;\n\n")

        # --- Caster effect frames ---
        if state["caster_frames"]:
            cw, ch = state["caster_w"], state["caster_h"]
            for idx, frame in enumerate(state["caster_frames"]):
                emit_pixel_array(lines, f"{symbol}_caster_{idx}_pixels", frame, cw, ch)
            lines.append(f"static const uint16_t * const {symbol}_caster_frames[] = {{\n")
            for idx in range(len(state["caster_frames"])):
                comma = "," if idx + 1 < len(state["caster_frames"]) else ""
                lines.append(f"  {symbol}_caster_{idx}_pixels{comma}\n")
            lines.append("};\n\n")

        # --- Projectile frames ---
        if state["proj_frames"]:
            pw, ph = state["proj_w"], state["proj_h"]
            for idx, frame in enumerate(state["proj_frames"]):
                emit_pixel_array(lines, f"{symbol}_proj_{idx}_pixels", frame, pw, ph)
            lines.append(f"static const uint16_t * const {symbol}_proj_frames[] = {{\n")
            for idx in range(len(state["proj_frames"])):
                comma = "," if idx + 1 < len(state["proj_frames"]) else ""
                lines.append(f"  {symbol}_proj_{idx}_pixels{comma}\n")
            lines.append("};\n\n")

    # --- Animation table ---
    lines.append(
        "const NarutoFullNineTailsMoveAnimation"
        " naruto_full_nine_tails_move_animations[NARUTO_FULL_NINE_TAILS_MOVE_STATE_COUNT] = {\n"
    )

    for idx, state in enumerate(states):
        comma = "," if idx + 1 < len(states) else ""
        symbol = state["symbol"]
        frames_ptr = f"{symbol}_frames" if state["frames"] else "0"

        # caster effect
        if state["caster_frames"]:
            ce_ptr = f"{symbol}_caster_frames"
            ce_cnt = f"{len(state['caster_frames'])}U"
            ce_start = f"{state['caster_start']}U"
            ce_loop = f"{state['caster_loop']}U"
            ce_w = f"{state['caster_w']}U"
            ce_h = f"{state['caster_h']}U"
        else:
            ce_ptr, ce_cnt, ce_start, ce_loop, ce_w, ce_h = "0", "0U", "0U", "0U", "0U", "0U"

        # projectile
        if state["proj_frames"]:
            pr_ptr = f"{symbol}_proj_frames"
            pr_cnt = f"{len(state['proj_frames'])}U"
            pr_loop = f"{state['proj_loop']}U"
            pr_w = f"{state['proj_w']}U"
            pr_h = f"{state['proj_h']}U"
        else:
            pr_ptr, pr_cnt, pr_loop, pr_w, pr_h = "0", "0U", "0U", "0U", "0U"

        lines.append(
            f"  [{state['macro']}] = {{\n"
            f"    {frames_ptr}, {len(state['frames'])}U, {state['loop']}U, {state['hold_last']}U,\n"
            f"    {ce_ptr}, {ce_cnt}, {ce_start}, {ce_loop}, {ce_w}, {ce_h},\n"
            f"    {pr_ptr}, {pr_cnt}, {pr_loop}, {pr_w}, {pr_h}\n"
            f"  }}{comma}\n"
        )

    lines.append("};\n")

    OUT_C.write_text("".join(lines), encoding="ascii")


def main():
    print("Loading moveset...")
    states = load_moveset()

    print("Writing header...")
    write_header(states)

    print("Writing source...")
    write_source(states)

    total_char_frames = sum(len(s["frames"]) for s in states)
    print(f"\nGenerated {OUT_H.relative_to(ROOT)}")
    print(f"Generated {OUT_C.relative_to(ROOT)}")
    print(f"{len(states)} states, {total_char_frames} total character frame entries")
    print()
    for state in states:
        ce_info = f"  caster={len(state['caster_frames'])}f ({state['caster_w']}x{state['caster_h']})" if state["caster_frames"] else ""
        pr_info = f"  proj={len(state['proj_frames'])}f ({state['proj_w']}x{state['proj_h']})" if state["proj_frames"] else ""
        print(f"  {state['name']}: {len(state['frames'])} char frames{ce_info}{pr_info}")


if __name__ == "__main__":
    main()
