import os
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
INPUT_DIR = ROOT / "assets" / "raw" / "chidori"
OUT_H = ROOT / "Core" / "Inc" / "chidori_data.h"
OUT_C = ROOT / "Core" / "Src" / "chidori_data.c"

COLOR_KEY = 0xF81F

def rgb565(r, g, b):
    value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    if value == COLOR_KEY:
        return 0xF81E
    return value

def load_frame(path):
    img = Image.open(path).convert("RGBA")
    width, height = img.size
    pixels = []
    
    for r, g, b, a in img.getdata():
        if a < 128:
            pixels.append(COLOR_KEY)
        else:
            pixels.append(rgb565(r, g, b))
            
    return width, height, pixels

def main():
    print("Converting Chidori frames to RGB565 C code...")
    
    h_lines = [
        """/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : chidori_data.h
  * @brief          : Chidori effects data structures and declarations.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __CHIDORI_DATA_H
#define __CHIDORI_DATA_H

#include <stdint.h>

typedef struct
{
  const uint16_t *pixels;
  uint16_t width;
  uint16_t height;
} ChidoriFrame;

extern const ChidoriFrame chidori_1_frames[4];
extern const ChidoriFrame chidori_2_frames[3];
extern const ChidoriFrame chidori_3_frames[3];
extern const ChidoriFrame chidori_4_frames[2];

#endif /* __CHIDORI_DATA_H */
"""
    ]
    
    c_lines = [
        """/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : chidori_data.c
  * @brief          : Chidori effects data definitions.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "chidori_data.h"

"""
    ]
    
    sheets = [
        ("chidori_1", 4),
        ("chidori_2", 3),
        ("chidori_3", 3),
        ("chidori_4", 2)
    ]
    
    for prefix, count in sheets:
        for idx in range(count):
            name = f"{prefix}_{idx}"
            path = INPUT_DIR / f"{name}.png"
            width, height, pixels = load_frame(path)
            
            c_lines.append(
                f"static const uint16_t {name}_pixels[{width}U * {height}U] = {{\n"
            )
            
            for i in range(0, len(pixels), 12):
                row = ", ".join(f"0x{val:04X}U" for val in pixels[i : i + 12])
                comma = "," if i + 12 < len(pixels) else ""
                c_lines.append(f"  {row}{comma}\n")
                
            c_lines.append("};\n\n")
            
        c_lines.append(f"const ChidoriFrame {prefix}_frames[{count}] = {{\n")
        for idx in range(count):
            name = f"{prefix}_{idx}"
            path = INPUT_DIR / f"{name}.png"
            width, height, _ = load_frame(path)
            comma = "," if idx + 1 < count else ""
            c_lines.append(
                f"  {{ {name}_pixels, {width}U, {height}U }}{comma}\n"
            )
        c_lines.append("};\n\n")
        
    OUT_H.write_text("".join(h_lines), encoding="ascii")
    OUT_C.write_text("".join(c_lines), encoding="ascii")
    print(f"Generated {OUT_H}")
    print(f"Generated {OUT_C}")

if __name__ == "__main__":
    main()
