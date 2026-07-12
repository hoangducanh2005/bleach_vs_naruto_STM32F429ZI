from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import json

manifest_path = Path("assets/vizard_ichigo_frames/manifest.json")
output_sheet_path = Path("assets/vizard_ichigo_frames/contact_sheet.png")

if not manifest_path.exists():
    print("Manifest not found")
    exit(1)

frames = json.loads(manifest_path.read_text(encoding="utf-8"))
num_frames = len(frames)
print(f"Generating contact sheet for {num_frames} frames...")

# We will lay out the frames in a grid.
# Let's say 10 columns.
cols = 10
rows = (num_frames + cols - 1) // cols

# Frame cell size
cell_w = 160
cell_h = 160

# Sheet size
sheet_w = cols * cell_w
sheet_h = rows * cell_h

# Create a sheet with gray checkerboard background
sheet = Image.new("RGBA", (sheet_w, sheet_h), (34, 34, 34, 255))
draw = ImageDraw.Draw(sheet)

# Draw checkerboard patterns on cells to help see transparency
for r in range(rows):
    for c in range(cols):
        cell_x = c * cell_w
        cell_y = r * cell_h
        
        # Draw cell border
        draw.rectangle([cell_x, cell_y, cell_x + cell_w - 1, cell_y + cell_h - 1], outline=(68, 68, 68, 255))
        
        # Checkerboard background inside the cell
        sub_size = 10
        for y in range(5, cell_h - 25, sub_size):
            for x in range(5, cell_w - 5, sub_size):
                if ((x // sub_size) + (y // sub_size)) % 2 == 0:
                    draw.rectangle([cell_x + x, cell_y + y, cell_x + x + sub_size - 1, cell_y + y + sub_size - 1], fill=(51, 51, 51, 255))
                else:
                    draw.rectangle([cell_x + x, cell_y + y, cell_x + x + sub_size - 1, cell_y + y + sub_size - 1], fill=(60, 60, 60, 255))

for idx, item in enumerate(frames):
    r = idx // cols
    c = idx % cols
    
    cell_x = c * cell_w
    cell_y = r * cell_h
    
    # Load frame
    frame_path = Path("assets/vizard_ichigo_frames") / item["file"]
    if frame_path.exists():
        frame_img = Image.open(frame_path)
        
        # Center the frame in the cell (leave space at bottom for text)
        fw, fh = frame_img.size
        # scale down if too large
        if fw > cell_w - 10 or fh > cell_h - 30:
            scale = min((cell_w - 10)/fw, (cell_h - 30)/fh)
            fw = int(fw * scale)
            fh = int(fh * scale)
            frame_img = frame_img.resize((fw, fh), Image.Resampling.LANCZOS)
            
        px = cell_x + (cell_w - fw) // 2
        py = cell_y + (cell_h - 30 - fh) // 2
        
        sheet.paste(frame_img, (px, py), frame_img)
        
    # Draw label text (frame index and size)
    label = f"Frame {idx}\n{item['w']}x{item['h']}"
    # Use default font
    draw.text((cell_x + 5, cell_y + cell_h - 25), label, fill=(200, 200, 200, 255))

sheet.save(output_sheet_path)
print(f"Saved contact sheet to {output_sheet_path}")
