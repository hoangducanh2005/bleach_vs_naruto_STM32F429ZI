from PIL import Image
import numpy as np

img_path = r"C:\Users\Admin\Downloads\vizard_ichigo_sprite_by_yurestu_d3ahl60-375w-2x.jpg"
try:
    img = Image.open(img_path)
    print("Size:", img.size)
    print("Mode:", img.mode)
    # Get top-left pixel
    tl = img.getpixel((0,0))
    print("Top-left pixel:", tl)
    
    # Get a sample of the background colors (e.g. from the first row)
    first_row = [img.getpixel((x, 0)) for x in range(img.width)]
    unique_colors = set(first_row)
    print("Number of unique colors in first row:", len(unique_colors))
    # Print some unique colors
    print("Sample colors in first row:", list(unique_colors)[:10])
except Exception as e:
    print("Error:", e)
