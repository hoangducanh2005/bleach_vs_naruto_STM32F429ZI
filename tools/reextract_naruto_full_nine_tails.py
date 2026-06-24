"""
Re-extract problematic Naruto Full Nine Tails strips with per-strip tuned parameters.

Problems identified:
  Merged frames:   strips 1, 2, 6, 11, 13 (DILATE_SIZE too large -> groups merge)
  Merged+missing:  strips 9, 12, 15 (both merging and some frames missed)
  Incomplete cuts: strips 4, 7 (MAX_FRAME_WIDTH/HEIGHT too small)
  Special:         strip 21 -> user wants exactly 2 frames (half of 20 arms each)
"""

from collections import deque
import csv
import json
from pathlib import Path
from PIL import Image, ImageFilter

ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIR = ROOT / ".." / "Naruto_Full_nine_tails"
OUTPUT_ROOT = ROOT / "assets" / "raw" / "naruto_full_nine_tails_frames"

BACKGROUND = (0, 96, 0)


# ---------------------------------------------------------------------------
# Per-strip overrides.  Keys not listed here fall back to DEFAULTS.
# ---------------------------------------------------------------------------
DEFAULTS = dict(dilate=3, min_px=18, min_area=100, max_w=200, max_h=200)

STRIP_CONFIG = {
    # merged + small gaps -> use dilate=1 so neighbours don't bleed together
    1:  dict(dilate=1, min_px=18, min_area=100, max_w=200, max_h=200),
    2:  dict(dilate=1, min_px=18, min_area=100, max_w=200, max_h=200),
    6:  dict(dilate=1, min_px=18, min_area=100, max_w=200, max_h=200),
    9:  dict(dilate=1, min_px=18, min_area=100, max_w=400, max_h=200),  # region 0 is 235px wide
    11: dict(dilate=1, min_px=18, min_area=100, max_w=200, max_h=200),
    12: dict(dilate=1, min_px=18, min_area=100, max_w=200, max_h=200),
    13: dict(dilate=1, min_px=18, min_area=100, max_w=200, max_h=200),
    15: dict(dilate=1, min_px=18, min_area=100, max_w=200, max_h=200),
    # incomplete cuts -> allow wider/taller frames
    4:  dict(dilate=3, min_px=18, min_area=100, max_w=400, max_h=400),
    7:  dict(dilate=3, min_px=18, min_area=100, max_w=400, max_h=400),
    # strip 21 handled separately (special_strip_21)
}


# ---------------------------------------------------------------------------
# Core helpers (copied & adapted from extract_naruto_full_nine_tails_frames.py)
# ---------------------------------------------------------------------------

def is_foreground(pixel):
    return pixel[:3] != BACKGROUND


def connected_components(mask):
    width, height = mask.size
    data = mask.load()
    visited = bytearray(width * height)
    components = []
    for y in range(height):
        for x in range(width):
            idx = y * width + x
            if visited[idx] or data[x, y] == 0:
                continue
            queue = deque([(x, y)])
            visited[idx] = 1
            min_x = max_x = x
            min_y = max_y = y
            pixels = 0
            while queue:
                cx, cy = queue.popleft()
                pixels += 1
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
                        nidx = ny * width + nx
                        if visited[nidx] or data[nx, ny] == 0:
                            continue
                        visited[nidx] = 1
                        queue.append((nx, ny))
            components.append((min_x, min_y, max_x + 1, max_y + 1, pixels))
    return components


def tighten_bbox(image, bbox):
    left, top, right, bottom = bbox
    crop = image.crop((left, top, right, bottom))
    pix = crop.load()
    w, h = crop.size
    min_x, min_y, max_x, max_y = w, h, -1, -1
    for y in range(h):
        for x in range(w):
            if is_foreground(pix[x, y]):
                min_x = min(min_x, x)
                max_x = max(max_x, x)
                min_y = min(min_y, y)
                max_y = max(max_y, y)
    if max_x < min_x:
        return None
    return (left + min_x, top + min_y, left + max_x + 1, top + max_y + 1)


def make_transparent_frame(image, bbox):
    frame = image.crop(bbox).convert("RGBA")
    pix = frame.load()
    w, h = frame.size
    for y in range(h):
        for x in range(w):
            r, g, b, *_ = pix[x, y]
            if r == 0 and g == 96 and b == 0:
                pix[x, y] = (0, 0, 0, 0)
    return frame


def save_frames(frames, output_subdir, strip_idx, image):
    """Save extracted frames, manifest.csv and manifest.json, then regenerate preview."""
    # Clear old files
    for old in output_subdir.glob(f"naruto_full_nine_tails_{strip_idx}_*.png"):
        old.unlink()
    output_subdir.mkdir(parents=True, exist_ok=True)

    for index, item in enumerate(frames):
        frame = make_transparent_frame(image, item["bbox"])
        filename = f"naruto_full_nine_tails_{strip_idx}_{index:03d}.png"
        frame.save(output_subdir / filename)
        item["file"] = filename

    with (output_subdir / "manifest.csv").open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=["file", "x", "y", "w", "h"])
        writer.writeheader()
        for item in frames:
            writer.writerow({k: item[k] for k in ["file", "x", "y", "w", "h"]})

    with (output_subdir / "manifest.json").open("w", encoding="utf-8") as f:
        json.dump(frames, f, indent=2)

    print(f"Strip {strip_idx:2d}: saved {len(frames)} frames to {output_subdir.name}")


def generate_preview(output_subdir, strip_idx):
    import math
    from PIL import ImageDraw, ImageFont

    files = sorted(output_subdir.glob(f"naruto_full_nine_tails_{strip_idx}_*.png"))
    if not files:
        return
    loaded = [(f, Image.open(f)) for f in files]
    max_w = max(img.size[0] for _, img in loaded)
    max_h = max(img.size[1] for _, img in loaded)
    cell_w, cell_h = max_w + 16, max_h + 16
    cols = min(8, len(loaded))
    rows = math.ceil(len(loaded) / cols)
    grid = Image.new("RGB", (cols * cell_w, rows * cell_h), (220, 220, 220))
    draw = ImageDraw.Draw(grid)
    try:
        font = ImageFont.load_default()
    except Exception:
        font = None
    for idx, (fpath, img) in enumerate(loaded):
        c, r = idx % cols, idx // cols
        xo, yo = c * cell_w, r * cell_h
        draw.rectangle([xo, yo, xo + cell_w - 1, yo + cell_h - 1], outline=(180, 180, 180))
        img_rgba = img.convert("RGBA")
        w, h = img_rgba.size
        bg = Image.new("RGBA", img_rgba.size, (255, 255, 255, 255))
        bg.alpha_composite(img_rgba)
        grid.paste(bg.convert("RGB"), (xo + (cell_w - w) // 2, yo + (cell_h - h) // 2))
        draw.text((xo + 4, yo + 4), str(int(fpath.stem.split("_")[-1])), fill=(0, 0, 0), font=font)
        img.close()
    out = output_subdir / f"preview_naruto_full_nine_tails_{strip_idx}.png"
    grid.save(out)
    print(f"          -> Preview saved: {out.name}")


# ---------------------------------------------------------------------------
# Standard re-extraction for a single strip
# ---------------------------------------------------------------------------

def reextract_strip(strip_idx):
    cfg = {**DEFAULTS, **STRIP_CONFIG.get(strip_idx, {})}
    dilate = cfg["dilate"]
    min_px = cfg["min_px"]
    min_area = cfg["min_area"]
    max_w = cfg["max_w"]
    max_h = cfg["max_h"]

    image_path = SOURCE_DIR / f"naruto_full_nine_tails_{strip_idx}.png"
    image = Image.open(image_path).convert("RGB")
    width, height = image.size

    base_mask = Image.new("L", image.size, 0)
    mask_pix = base_mask.load()
    src_pix = image.load()
    for y in range(height):
        for x in range(width):
            if is_foreground(src_pix[x, y]):
                mask_pix[x, y] = 255

    # Use dilate_size 1 -> MaxFilter(1) is essentially no dilation
    dilate_filter_size = max(1, dilate * 2 - 1)  # must be odd
    if dilate <= 1:
        grouped_mask = base_mask  # no dilation at all -> rely on exact gaps
    else:
        grouped_mask = base_mask.filter(ImageFilter.MaxFilter(dilate_filter_size))

    raw = connected_components(grouped_mask)
    frames = []
    for bl, bt, br, bb, px in raw:
        if px < min_px:
            continue
        tight = tighten_bbox(image, (bl, bt, br, bb))
        if tight is None:
            continue
        l, t, r, b = tight
        fw, fh = r - l, b - t
        if fw * fh < min_area:
            continue
        if fw > max_w or fh > max_h:
            continue
        frames.append({"bbox": tight, "x": l, "y": t, "w": fw, "h": fh})

    frames.sort(key=lambda item: item["x"])

    output_subdir = OUTPUT_ROOT / f"naruto_full_nine_tails_{strip_idx}"
    save_frames(frames, output_subdir, strip_idx, image)
    generate_preview(output_subdir, strip_idx)


# ---------------------------------------------------------------------------
# Special handler for strip 21 (20 individual arms -> 2 composite frames)
# ---------------------------------------------------------------------------

def special_strip_21():
    """
    Strip 21 has 20 'arm' sprites.  User wants exactly 2 frames:
      Frame 0 = arms 0..9  (first 10, leftmost half of the strip)
      Frame 1 = arms 10..19 (last 10, rightmost half)
    Strategy: split the strip roughly at the midpoint between arm 9 and arm 10.
    """
    strip_idx = 21
    image_path = SOURCE_DIR / f"naruto_full_nine_tails_{strip_idx}.png"
    image = Image.open(image_path).convert("RGB")
    width, height = image.size

    # The 20 arm start X positions found by analysis:
    arm_starts = [22, 97, 170, 250, 320, 390, 465, 529, 587, 646,
                  742, 800, 859, 924, 992, 1059, 1131, 1211, 1285, 1357]
    # Split between arm 9 (start=646) and arm 10 (start=742)
    # Use the midpoint as the column separator
    split_x = (arm_starts[9] + arm_starts[10]) // 2  # = 694

    # Frame 0: x=0..split_x, Frame 1: x=split_x..width
    bboxes = [
        (0, 0, split_x, height),
        (split_x, 0, width, height),
    ]

    frames = []
    for bbox in bboxes:
        tight = tighten_bbox(image, bbox)
        if tight is None:
            continue
        l, t, r, b = tight
        frames.append({"bbox": tight, "x": l, "y": t, "w": r - l, "h": b - t})

    output_subdir = OUTPUT_ROOT / f"naruto_full_nine_tails_{strip_idx}"
    save_frames(frames, output_subdir, strip_idx, image)
    generate_preview(output_subdir, strip_idx)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    standard_strips = [1, 2, 4, 6, 7, 9, 11, 12, 13, 15]
    for idx in standard_strips:
        reextract_strip(idx)

    special_strip_21()

    print("\nAll problematic strips re-extracted.")


if __name__ == "__main__":
    main()
