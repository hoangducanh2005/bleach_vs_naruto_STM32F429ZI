from collections import deque
import csv
import json
from pathlib import Path

from PIL import Image, ImageFilter

ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIR = ROOT / ".." / "Naruto_Full_nine_tails"
OUTPUT_ROOT = ROOT / "assets" / "raw" / "naruto_full_nine_tails_frames"

BACKGROUND = (0, 96, 0)
DILATE_SIZE = 9
MIN_PIXELS = 18
MIN_BBOX_AREA = 100
MAX_FRAME_WIDTH = 200
MAX_FRAME_HEIGHT = 200


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
            # Check if pixel is close to green background (allowing some tolerance)
            r, g, b, *a = pixels[x, y]
            # Color thresholding for safety (background is exactly 0, 96, 0)
            if r == 0 and g == 96 and b == 0:
                pixels[x, y] = (0, 0, 0, 0)

    return frame


def process_image(image_path, output_dir, file_index):
    if not image_path.exists():
        print(f"File {image_path} does not exist!")
        return 0

    image = Image.open(image_path).convert("RGB")
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

    # Sort frames mainly horizontally (from left to right) since they are in a strip
    frames.sort(key=lambda item: item["x"])

    # Prepare output subdirectory
    output_subdir = output_dir / f"naruto_full_nine_tails_{file_index}"
    if output_subdir.exists():
        for old_file in output_subdir.glob(f"naruto_full_nine_tails_{file_index}_*.png"):
            old_file.unlink()
    output_subdir.mkdir(parents=True, exist_ok=True)

    for index, item in enumerate(frames):
        frame = make_transparent_frame(image, item["bbox"])
        filename = f"naruto_full_nine_tails_{file_index}_{index:03d}.png"
        frame.save(output_subdir / filename)
        item["file"] = filename

    # Write manifest files inside the subdirectory
    if frames:
        with (output_subdir / "manifest.csv").open("w", newline="", encoding="utf-8") as csv_file:
            writer = csv.DictWriter(csv_file, fieldnames=["file", "x", "y", "w", "h"])
            writer.writeheader()
            for item in frames:
                writer.writerow({key: item[key] for key in ["file", "x", "y", "w", "h"]})

        with (output_subdir / "manifest.json").open("w", encoding="utf-8") as json_file:
            json.dump(frames, json_file, indent=2)

    print(f"Extracted {len(frames)} frames from strip {file_index} to {output_subdir.name}")
    return len(frames)


def main():
    total_frames = 0
    OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
    
    for i in range(1, 23):
        image_path = SOURCE_DIR / f"naruto_full_nine_tails_{i}.png"
        frames_count = process_image(image_path, OUTPUT_ROOT, i)
        total_frames += frames_count
        
    print(f"Finished. Extracted total {total_frames} frames from 22 strips.")


if __name__ == "__main__":
    main()
