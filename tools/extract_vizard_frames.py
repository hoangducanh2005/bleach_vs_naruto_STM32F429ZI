from collections import deque
import csv
import json
from pathlib import Path
from PIL import Image, ImageFilter

SOURCE = Path(r"C:\Users\Admin\Downloads\vizard_ichigo_sprite_by_yurestu_d3ahl60-375w-2x.jpg")
OUTPUT_DIR = Path("assets/vizard_ichigo_frames")
DILATE_SIZE = 11
MIN_PIXELS = 18
MIN_BBOX_AREA = 120
MAX_FRAME_WIDTH = 250
MAX_FRAME_HEIGHT = 250

def is_foreground(pixel):
    r, g, b = pixel[:3]
    # Background is green-dominated
    is_bg = (g > 50) and (g > r + 20) and (g > b + 20)
    return not is_bg

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
                if cx < min_x:
                    min_x = cx
                if cx > max_x:
                    max_x = cx
                if cy < min_y:
                    min_y = cy
                if cy > max_y:
                    max_y = cy

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
                if x < min_x:
                    min_x = x
                if x > max_x:
                    max_x = x
                if y < min_y:
                    min_y = y
                if y > max_y:
                    max_y = y

    if max_x < min_x:
        return None

    return (left + min_x, top + min_y, left + max_x + 1, top + max_y + 1)

def make_transparent_frame(image, bbox):
    frame = image.crop(bbox).convert("RGBA")
    pixels = frame.load()
    width, height = frame.size

    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            is_bg = (g > 50) and (g > r + 20) and (g > b + 20)
            if is_bg:
                pixels[x, y] = (0, 0, 0, 0)

    return frame

def main():
    image = Image.open(SOURCE).convert("RGB")
    width, height = image.size

    base_mask = Image.new("L", image.size, 0)
    mask_pixels = base_mask.load()
    src_pixels = image.load()
    for y in range(height):
        for x in range(width):
            if is_foreground(src_pixels[x, y]):
                mask_pixels[x, y] = 255

    grouped_mask = base_mask.filter(ImageFilter.MaxFilter(DILATE_SIZE))
    raw_components = connected_components(grouped_mask)

    frames = []
    for bbox_left, bbox_top, bbox_right, bbox_bottom, pixels in raw_components:
        if pixels < MIN_PIXELS:
            continue
        # Filter out credit box at the bottom
        if bbox_top > height - 190:
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

    # Sort frames by rows (using a tolerance of 30 pixels for row detection)
    # Cluster frames into rows and sort each row by X
    frames.sort(key=lambda item: item["y"])
    rows = []
    current_row = []
    last_y = -100
    for f in frames:
        if last_y == -100 or (f["y"] - last_y) < 30:
            current_row.append(f)
            last_y = sum(item["y"] for item in current_row) / len(current_row)
        else:
            rows.append(current_row)
            current_row = [f]
            last_y = f["y"]
    if current_row:
        rows.append(current_row)

    sorted_frames = []
    for r in rows:
        r.sort(key=lambda item: item["x"])
        sorted_frames.extend(r)
    frames = sorted_frames

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    for old_file in OUTPUT_DIR.glob("vizard_ichigo_*.png"):
        old_file.unlink()

    for index, item in enumerate(frames):
        frame = make_transparent_frame(image, item["bbox"])
        filename = f"vizard_ichigo_{index:03d}.png"
        frame.save(OUTPUT_DIR / filename)
        item["file"] = filename

    with (OUTPUT_DIR / "manifest.csv").open("w", newline="", encoding="utf-8") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=["file", "x", "y", "w", "h"])
        writer.writeheader()
        for item in frames:
            writer.writerow({key: item[key] for key in ["file", "x", "y", "w", "h"]})

    with (OUTPUT_DIR / "manifest.json").open("w", encoding="utf-8") as json_file:
        json.dump(frames, json_file, indent=2)

    print(f"Extracted {len(frames)} frames to {OUTPUT_DIR}")

if __name__ == "__main__":
    main()
