import json
from pathlib import Path

manifest_path = Path("assets/vizard_ichigo_frames/manifest.json")
if manifest_path.exists():
    frames = json.loads(manifest_path.read_text(encoding="utf-8"))
    print(f"Total frames: {len(frames)}")
    
    # Let's group frames by Y coordinate.
    # We can cluster them if Y coordinates are within, say, 25 pixels of each other.
    rows = []
    current_row = []
    last_y = -100
    
    # First sort by Y then by X
    frames_sorted = sorted(frames, key=lambda f: (f["y"], f["x"]))
    
    for f in frames_sorted:
        if last_y == -100 or (f["y"] - last_y) > 30:
            if current_row:
                rows.append(current_row)
            current_row = [f]
            last_y = f["y"]
        else:
            current_row.append(f)
            # update last_y to be the average or just keep current
            last_y = (last_y * len(current_row) + f["y"]) / (len(current_row) + 1)
            
    if current_row:
        rows.append(current_row)
        
    print(f"Detected {len(rows)} rows of frames:")
    for idx, row in enumerate(rows):
        avg_y = sum(f["y"] for f in row) / len(row)
        print(f"Row {idx+1}: {len(row)} frames, avg_y={avg_y:.1f}, x_range=[{row[0]['x']}, {row[-1]['x']}]")
        for f in row:
            print(f"  Frame {f['file']}: x={f['x']}, y={f['y']}, w={f['w']}, h={f['h']}")
else:
    print("Manifest not found")
