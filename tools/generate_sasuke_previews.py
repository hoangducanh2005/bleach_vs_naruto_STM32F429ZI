import math
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
FRAMES_DIR = ROOT / "assets" / "raw" / "sasuke_frames"
OUTPUT_PATH = FRAMES_DIR / "preview_sasuke_moveset.png"

def main():
    # Find all sasuke_*.png files
    files = sorted(list(FRAMES_DIR.glob("sasuke_*.png")))
    num_frames = len(files)
    if num_frames == 0:
        print("No frames found in", FRAMES_DIR)
        return

    # Grid calculations
    cols = 15
    rows = math.ceil(num_frames / cols)
    
    cell_w, cell_h = 80, 80
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

    for idx, filepath in enumerate(files):
        # Extract index number from filename like sasuke_005.png
        filename = filepath.name
        try:
            frame_num = int(filepath.stem.split("_")[1])
        except Exception:
            frame_num = idx

        # Calculate position
        c = idx % cols
        r = idx // cols
        x_offset = c * cell_w
        y_offset = r * cell_h

        # Draw cell border
        draw.rectangle(
            [x_offset, y_offset, x_offset + cell_w - 1, y_offset + cell_h - 1],
            outline=(180, 180, 180)
        )

        # Open and paste the frame
        with Image.open(filepath) as img:
            # Convert to RGBA to preserve transparency if pasting onto RGB
            img_rgba = img.convert("RGBA")
            # Center the frame in the cell
            w, h = img_rgba.size
            px = x_offset + (cell_w - w) // 2
            py = y_offset + (cell_h - h) // 2
            
            # Create a white background for the character to stand out
            char_bg = Image.new("RGBA", img_rgba.size, (255, 255, 255, 255))
            char_bg.alpha_composite(img_rgba)
            
            grid_img.paste(char_bg.convert("RGB"), (px, py))

        # Draw index label
        draw.text(
            (x_offset + 4, y_offset + 4),
            str(frame_num),
            fill=(0, 0, 0),
            font=font
        )

    grid_img.save(OUTPUT_PATH)
    print(f"Generated preview grid of {num_frames} frames to {OUTPUT_PATH}")

if __name__ == "__main__":
    main()
