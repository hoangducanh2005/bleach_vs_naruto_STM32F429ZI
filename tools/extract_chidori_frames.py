import os
from collections import deque
from pathlib import Path
from PIL import Image, ImageFilter

ROOT = Path(__file__).resolve().parents[1]
SASUKE_DIR = ROOT.parent / "Sasuke"
OUTPUT_DIR = ROOT / "assets" / "raw" / "chidori"

BACKGROUND = (0, 128, 0)
DILATE_SIZE = 7
MIN_PIXELS = 10

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
            if pixels[x, y][:3] == BACKGROUND:
                pixels[x, y] = (0, 0, 0, 0)

    return frame

def extract_chidori_sheet(filename, prefix):
    path = SASUKE_DIR / filename
    if not path.exists():
        print(f"File {path} not found")
        return []

    image = Image.open(path).convert("RGB")
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

        tight = tighten_bbox(image, (bbox_left, bbox_top, bbox_right, bbox_bottom))
        if tight is None:
            continue

        frames.append(tight)

    # Sort components from left to right
    frames.sort(key=lambda bbox: bbox[0])

    os.makedirs(OUTPUT_DIR, exist_ok=True)
    
    # Delete old matching files
    for old_file in OUTPUT_DIR.glob(f"{prefix}_*.png"):
        old_file.unlink()

    saved_files = []
    for index, bbox in enumerate(frames):
        frame = make_transparent_frame(image, bbox)
        filename_out = f"{prefix}_{index}.png"
        out_path = OUTPUT_DIR / filename_out
        frame.save(out_path)
        saved_files.append(out_path)
        print(f"  Saved {out_path.relative_to(ROOT)} size {frame.size}")

    return saved_files

def main():
    print("Extracting Chidori frames using connected components (green background filter)...")
    extract_chidori_sheet("Chidori_1.png", "chidori_1")
    extract_chidori_sheet("Chidori_2.png", "chidori_2")
    extract_chidori_sheet("Chidori_3.png", "chidori_3")
    extract_chidori_sheet("Chidori_4.png", "chidori_4")
    print("All Chidori sheets successfully extracted!")

if __name__ == "__main__":
    main()
