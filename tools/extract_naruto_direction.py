import csv
import json
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
INPUT = ROOT / "assets" / "naruto_direction.png"
OUTPUT_DIR = ROOT / "assets" / "naruto_direction"
FRAME_DIR = OUTPUT_DIR / "frames"
BACKGROUND_TOLERANCE = 20
MIN_COMPONENT_AREA = 80
MIN_COMPONENT_WIDTH = 12
MIN_COMPONENT_HEIGHT = 12
PADDING = 2
MERGE_GAP_X = 8


def is_foreground(pixel, background):
    r, g, b, a = pixel
    if a < 128:
        return False
    return (
        abs(r - background[0])
        + abs(g - background[1])
        + abs(b - background[2])
    ) > BACKGROUND_TOLERANCE


def find_components(image):
    width, height = image.size
    pixels = image.load()
    background = image.getpixel((0, 0))[:3]
    mask = [False] * (width * height)

    for y in range(height):
        for x in range(width):
            mask[y * width + x] = is_foreground(pixels[x, y], background)

    seen = [False] * (width * height)
    components = []

    for start, active in enumerate(mask):
        if not active or seen[start]:
            continue

        stack = [start]
        seen[start] = True
        min_x = width
        min_y = height
        max_x = 0
        max_y = 0
        area = 0

        while stack:
            index = stack.pop()
            x = index % width
            y = index // width
            area += 1
            min_x = min(min_x, x)
            min_y = min(min_y, y)
            max_x = max(max_x, x)
            max_y = max(max_y, y)

            for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                if 0 <= nx < width and 0 <= ny < height:
                    nindex = ny * width + nx
                    if mask[nindex] and not seen[nindex]:
                        seen[nindex] = True
                        stack.append(nindex)

        box = (min_x, min_y, max_x + 1, max_y + 1)
        box_width = box[2] - box[0]
        box_height = box[3] - box[1]
        if (
            area >= MIN_COMPONENT_AREA
            and box_width >= MIN_COMPONENT_WIDTH
            and box_height >= MIN_COMPONENT_HEIGHT
        ):
            components.append({"box": box, "area": area})

    components = merge_nearby_components(components)
    components = sort_components_by_rows(components)
    return components, background


def vertical_overlap(a, b):
    top = max(a[1], b[1])
    bottom = min(a[3], b[3])
    return max(0, bottom - top)


def merge_boxes(a, b):
    return (
        min(a[0], b[0]),
        min(a[1], b[1]),
        max(a[2], b[2]),
        max(a[3], b[3]),
    )


def should_merge(a, b):
    a_box = a["box"]
    b_box = b["box"]
    a_height = a_box[3] - a_box[1]
    b_height = b_box[3] - b_box[1]
    overlap = vertical_overlap(a_box, b_box)
    if overlap < min(a_height, b_height) // 2:
        return False

    gap = max(a_box[0], b_box[0]) - min(a_box[2], b_box[2])
    return gap <= MERGE_GAP_X


def merge_nearby_components(components):
    changed = True
    merged = components[:]

    while changed:
        changed = False
        next_components = []
        used = [False] * len(merged)

        for i, component in enumerate(merged):
            if used[i]:
                continue

            current = component.copy()
            used[i] = True

            for j in range(i + 1, len(merged)):
                if used[j]:
                    continue
                if should_merge(current, merged[j]):
                    current["box"] = merge_boxes(current["box"], merged[j]["box"])
                    current["area"] += merged[j]["area"]
                    used[j] = True
                    changed = True

            next_components.append(current)

        merged = next_components

    return merged


def sort_components_by_rows(components):
    rows = []
    for component in sorted(components, key=lambda item: item["box"][1]):
        box = component["box"]
        placed = False
        for row in rows:
            row_top, row_bottom, items = row
            overlap = max(0, min(row_bottom, box[3]) - max(row_top, box[1]))
            if overlap >= min(row_bottom - row_top, box[3] - box[1]) // 2:
                items.append(component)
                row[0] = min(row_top, box[1])
                row[1] = max(row_bottom, box[3])
                placed = True
                break
        if not placed:
            rows.append([box[1], box[3], [component]])

    output = []
    for _top, _bottom, items in sorted(rows, key=lambda row: row[0]):
        output.extend(sorted(items, key=lambda item: item["box"][0]))
    return output


def crop_transparent(image, box, background):
    width, height = image.size
    left = max(0, box[0] - PADDING)
    top = max(0, box[1] - PADDING)
    right = min(width, box[2] + PADDING)
    bottom = min(height, box[3] + PADDING)
    crop = image.crop((left, top, right, bottom)).convert("RGBA")
    pixels = crop.load()

    for y in range(crop.height):
        for x in range(crop.width):
            if not is_foreground(pixels[x, y], background):
                pixels[x, y] = (0, 0, 0, 0)

    return crop, (left, top, right, bottom)


def make_contact_sheet(frames):
    if not frames:
        return

    cell_w = max(frame.width for frame in frames) + 12
    cell_h = max(frame.height for frame in frames) + 20
    cols = min(6, len(frames))
    rows = (len(frames) + cols - 1) // cols
    sheet = Image.new("RGBA", (cell_w * cols, cell_h * rows), (48, 48, 48, 255))

    for index, frame in enumerate(frames):
        col = index % cols
        row = index // cols
        x0 = col * cell_w
        y0 = row * cell_h
        for y in range(y0, y0 + cell_h):
            for x in range(x0, x0 + cell_w):
                shade = 96 if ((x // 8) + (y // 8)) % 2 == 0 else 128
                sheet.putpixel((x, y), (shade, shade, shade, 255))
        x = x0 + (cell_w - frame.width) // 2
        y = y0 + 14 + (cell_h - 14 - frame.height) // 2
        sheet.alpha_composite(frame, (x, y))

    sheet.save(OUTPUT_DIR / "contact_sheet.png")


def main():
    image = Image.open(INPUT).convert("RGBA")
    components, background = find_components(image)

    FRAME_DIR.mkdir(parents=True, exist_ok=True)
    for old_frame in FRAME_DIR.glob("naruto_direction_*.png"):
        old_frame.unlink()

    background_image = Image.new("RGB", image.size, background)
    background_image.save(OUTPUT_DIR / "background.png")

    manifest = []
    frames = []
    for index, component in enumerate(components):
        frame, padded_box = crop_transparent(image, component["box"], background)
        filename = f"naruto_direction_{index:03d}.png"
        frame.save(FRAME_DIR / filename)
        frames.append(frame)
        manifest.append(
            {
                "index": index,
                "file": f"frames/{filename}",
                "source_box": list(component["box"]),
                "padded_box": list(padded_box),
                "width": frame.width,
                "height": frame.height,
                "area": component["area"],
            }
        )

    (OUTPUT_DIR / "manifest.json").write_text(
        json.dumps(manifest, indent=2), encoding="utf-8"
    )
    with (OUTPUT_DIR / "manifest.csv").open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "index",
                "file",
                "source_box",
                "padded_box",
                "width",
                "height",
                "area",
            ],
        )
        writer.writeheader()
        writer.writerows(manifest)

    make_contact_sheet(frames)
    print(f"Background RGB: {background}")
    print(f"Extracted {len(frames)} frames to {FRAME_DIR.relative_to(ROOT)}")
    print(f"Wrote {OUTPUT_DIR.relative_to(ROOT) / 'manifest.json'}")
    print(f"Wrote {OUTPUT_DIR.relative_to(ROOT) / 'contact_sheet.png'}")


if __name__ == "__main__":
    main()
