from collections import deque
import csv
import json
import os
import sys
from pathlib import Path
from PIL import Image, ImageFilter

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8')

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets" / "raw" / "rukia_spritesheet.png"
OUTPUT_DIR = ROOT / "assets" / "rukia_frames"
BACKGROUND = (128, 128, 255)
DILATE_SIZE = 11
MIN_PIXELS = 18
MIN_BBOX_AREA = 120
MAX_FRAME_WIDTH = 170
MAX_FRAME_HEIGHT = 170

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
    pixels = crop.load()
    width, height = crop.size
    min_x = width
    min_y = height
    max_x = -1
    max_y = -1

    for y in range(height):
        for x in range(width):
            if is_foreground(pixels[x, y]):
                min_x = min(min_x, x)
                max_x = max(max_x, x)
                min_y = min(min_y, y)
                max_y = max(max_y, y)

    if max_x < min_x:
        return None

    return (left + min_x, top + min_y, left + max_x + 1, top + max_y + 1)

def make_transparent_frame(image, bbox):
    frame = image.crop(bbox).convert("RGBA")
    pixels = frame.load()
    width, height = frame.size

    for y in range(height):
        for x in range(width):
            if pixels[x, y][:3] == BACKGROUND:
                pixels[x, y] = (0, 0, 0, 0)

    return frame

def main():
    print(f"Loading sprite sheet from {SOURCE}...")
    image = Image.open(SOURCE).convert("RGB")
    width, height = image.size

    base_mask = Image.new("L", image.size, 0)
    mask_pixels = base_mask.load()
    src_pixels = image.load()
    for y in range(height):
        for x in range(width):
            if is_foreground(src_pixels[x, y]):
                mask_pixels[x, y] = 255

    print("Grouping components with dilation...")
    grouped_mask = base_mask.filter(ImageFilter.MaxFilter(DILATE_SIZE))
    
    print("Finding connected components...")
    raw_components = connected_components(grouped_mask)

    frames = []
    for bbox_left, bbox_top, bbox_right, bbox_bottom, pixels in raw_components:
        if pixels < MIN_PIXELS:
            continue
        # Ignore credit watermark at bottom right of sheet
        if bbox_left > width - 175 and bbox_top > height - 115:
            continue
        # Also ignore the top header area (credits/title of sheet)
        if bbox_top < 60: # typically the first 60px of JUS sheet contains title
            continue

        tight = tighten_bbox(image, (bbox_left, bbox_top, bbox_right, bbox_bottom))
        if tight is None:
            continue

        left, top, right, bottom = tight
        frame_width = right - left
        frame_height = bottom - top
        if frame_width * frame_height < MIN_BBOX_AREA:
            continue
        if frame_width > MAX_FRAME_WIDTH or frame_height > MAX_FRAME_HEIGHT:
            continue

        frames.append(
            {
                "bbox": tight,
                "x": left,
                "y": top,
                "w": frame_width,
                "h": frame_height,
            }
        )

    # Sort frames row-by-row (with vertical spacing tolerance of 24px)
    frames.sort(key=lambda item: (item["y"] // 24, item["x"]))

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    for old_file in OUTPUT_DIR.glob("rukia_*.png"):
        old_file.unlink()

    print(f"Writing {len(frames)} transparent frames...")
    for index, item in enumerate(frames):
        frame = make_transparent_frame(image, item["bbox"])
        filename = f"rukia_{index:03d}.png"
        frame.save(OUTPUT_DIR / filename)
        item["file"] = filename

    with (OUTPUT_DIR / "manifest.csv").open("w", newline="", encoding="utf-8") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=["file", "x", "y", "w", "h"])
        writer.writeheader()
        for item in frames:
            writer.writerow({key: item[key] for key in ["file", "x", "y", "w", "h"]})

    with (OUTPUT_DIR / "manifest.json").open("w", encoding="utf-8") as json_file:
        json.dump(frames, json_file, indent=2)

    print(f"Successfully extracted {len(frames)} frames to {OUTPUT_DIR}")

if __name__ == "__main__":
    main()
