import os
import sys

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8')

out_dir = r"d:\File học tập\2025.2\Hệ nhúng\Project\assets\rukia_frames"
html_path = os.path.join(out_dir, "index.html")

files = sorted([f for f in os.listdir(out_dir) if f.endswith('.png')])

html_content = """
<!DOCTYPE html>
<html>
<head>
    <title>Extracted Rukia Frames</title>
    <style>
        body { font-family: sans-serif; background: #222; color: #fff; padding: 20px; }
        .gallery { display: flex; flex-wrap: wrap; gap: 10px; }
        .item { background: #333; border: 1px solid #444; padding: 8px; border-radius: 4px; text-align: center; }
        .item img { max-height: 100px; display: block; margin: 0 auto 5px; background: #111; border: 1px dashed #555; }
        .name { font-size: 11px; color: #aaa; }
    </style>
</head>
<body>
    <h1>Extracted Rukia Frames (""" + str(len(files)) + """ frames)</h1>
    <div class="gallery">
"""

for f in files:
    if f == "index.html":
        continue
    html_content += f"""
        <div class="item">
            <img src="{f}" />
            <div class="name">{f}</div>
        </div>
    """

html_content += """
    </div>
</body>
</html>
"""

with open(html_path, "w", encoding="utf-8") as html_file:
    html_file.write(html_content)

print(f"Created contact sheet for Rukia frames at {html_path}")
