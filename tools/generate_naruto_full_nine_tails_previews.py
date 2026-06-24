import math
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
FRAMES_ROOT = ROOT / "assets" / "raw" / "naruto_full_nine_tails_frames"


def generate_preview_for_subdir(subdir_path, subdir_index):
    # Find all png files matching the pattern
    files = sorted(list(subdir_path.glob(f"naruto_full_nine_tails_{subdir_index}_*.png")))
    num_frames = len(files)
    if num_frames == 0:
        return

    # Load images to find max width and height for dynamic grid cell sizing
    loaded_imgs = []
    max_w, max_h = 0, 0
    for filepath in files:
        img = Image.open(filepath)
        loaded_imgs.append((filepath, img))
        w, h = img.size
        if w > max_w:
            max_w = w
        if h > max_h:
            max_h = h

    # Add margin
    cell_w = max_w + 16
    cell_h = max_h + 16

    # Grid dimensions: keep max 8 columns or equal to num_frames if smaller
    cols = min(8, num_frames)
    rows = math.ceil(num_frames / cols)
    
    grid_w = cols * cell_w
    grid_h = rows * cell_h

    # Create grid image
    grid_img = Image.new("RGB", (grid_w, grid_h), (220, 220, 220))
    draw = ImageDraw.Draw(grid_img)

    # Use default font
    try:
        font = ImageFont.load_default()
    except Exception:
        font = None

    for idx, (filepath, img) in enumerate(loaded_imgs):
        # Extract index number from filename like naruto_full_nine_tails_1_005.png
        try:
            frame_num = int(filepath.stem.split("_")[-1])
        except Exception:
            frame_num = idx

        # Calculate grid position
        c = idx % cols
        r = idx // cols
        x_offset = c * cell_w
        y_offset = r * cell_h

        # Draw cell border
        draw.rectangle(
            [x_offset, y_offset, x_offset + cell_w - 1, y_offset + cell_h - 1],
            outline=(180, 180, 180)
        )

        # Paste the frame
        img_rgba = img.convert("RGBA")
        w, h = img_rgba.size
        px = x_offset + (cell_w - w) // 2
        py = y_offset + (cell_h - h) // 2
        
        # Create a white background for visibility
        char_bg = Image.new("RGBA", img_rgba.size, (255, 255, 255, 255))
        char_bg.alpha_composite(img_rgba)
        
        grid_img.paste(char_bg.convert("RGB"), (px, py))
        img.close()

        # Draw index label
        draw.text(
            (x_offset + 4, y_offset + 4),
            str(frame_num),
            fill=(0, 0, 0),
            font=font
        )

    output_path = subdir_path / f"preview_naruto_full_nine_tails_{subdir_index}.png"
    grid_img.save(output_path)
    print(f"Generated preview of {num_frames} frames to {output_path.name}")


def main():
    if not FRAMES_ROOT.exists():
        print(f"Frames directory {FRAMES_ROOT} does not exist!")
        return

    # Process all subdirectories from 1 to 22
    for i in range(1, 23):
        subdir = FRAMES_ROOT / f"naruto_full_nine_tails_{i}"
        if subdir.exists() and subdir.is_dir():
            generate_preview_for_subdir(subdir, i)


if __name__ == "__main__":
    main()
