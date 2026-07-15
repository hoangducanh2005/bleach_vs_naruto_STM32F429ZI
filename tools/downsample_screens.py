import os
import re

def downsample_file(c_path, h_path, w_old, h_old, prefix):
    w_new = w_old // 2
    h_new = h_old // 2
    size_new = w_new * h_new * 2

    # Read C file
    with open(c_path, 'r', encoding='utf-8') as f:
        c_content = f.read()

    # Find the array mapping bytes
    match = re.search(r'uint8_t\s+' + prefix + r'_map\s*\[[^\]]*\]\s*=\s*\{([^}]+)\}', c_content)
    if not match:
        match = re.search(r'uint8_t\s+' + prefix + r'_map\s*\[\s*\]\s*=\s*\{([^}]+)\}', c_content)
    
    if not match:
        print(f"Error: Could not find array {prefix}_map in {c_path}")
        return False

    bytes_str = match.group(1)
    # Parse all bytes
    byte_tokens = re.findall(r'0x[0-9a-fA-F]+|[0-9]+', bytes_str)
    
    raw_bytes = [int(tok, 0) for tok in byte_tokens]
    
    expected_len = w_old * h_old * 2
    if len(raw_bytes) < expected_len:
        print(f"Error: Parsed byte array length {len(raw_bytes)} is less than expected {expected_len} in {c_path}")
        return False

    # Downsample
    new_bytes = []
    for y in range(h_new):
        for x in range(w_new):
            src_y = y * 2
            src_x = x * 2
            src_idx = (src_y * w_old + src_x) * 2
            new_bytes.append(raw_bytes[src_idx])
            new_bytes.append(raw_bytes[src_idx + 1])

    # Format new bytes
    formatted_rows = []
    for i in range(0, len(new_bytes), 16):
        row_slice = new_bytes[i:i+16]
        row_str = ", ".join(f"0x{b:02x}" for b in row_slice)
        formatted_rows.append("  " + row_str)
    new_bytes_str = ",\n".join(formatted_rows)

    # Reconstruct C file
    original_decl = c_content[match.start():match.end()]
    decl_prefix = original_decl.split('{')[0]
    
    new_c_content = c_content
    new_c_content = re.sub(prefix.upper() + r'_WIDTH\s+\d+', f"{prefix.upper()}_WIDTH  {w_new}", new_c_content, flags=re.IGNORECASE)
    new_c_content = re.sub(prefix.upper() + r'_HEIGHT\s+\d+', f"{prefix.upper()}_HEIGHT {h_new}", new_c_content, flags=re.IGNORECASE)
    new_c_content = re.sub(prefix.upper() + r'_SIZE\s+\d+', f"{prefix.upper()}_SIZE   {size_new}", new_c_content, flags=re.IGNORECASE)
    
    new_array_def = f"{decl_prefix}{{\n{new_bytes_str}\n}}"
    new_c_content = new_c_content.replace(original_decl, new_array_def)
    
    with open(c_path, 'w', encoding='utf-8') as f:
        f.write(new_c_content)

    print(f"Downsampled C file: {c_path} successfully. New size: {size_new} bytes.")

    # Read and update Header file
    with open(h_path, 'r', encoding='utf-8') as f:
        h_content = f.read()

    new_h_content = h_content
    new_h_content = re.sub(prefix.upper() + r'_WIDTH\s+\d+U?', f"{prefix.upper()}_WIDTH   {w_new}U", new_h_content, flags=re.IGNORECASE)
    new_h_content = re.sub(prefix.upper() + r'_HEIGHT\s+\d+U?', f"{prefix.upper()}_HEIGHT  {h_new}U", new_h_content, flags=re.IGNORECASE)
    new_h_content = re.sub(prefix.upper() + r'_SIZE\s+\d+U?', f"{prefix.upper()}_SIZE    {size_new}U", new_h_content, flags=re.IGNORECASE)

    # For BLEACH_VS_NARUTO_SPLASH which doesn't have U suffix in defines
    new_h_content = re.sub(r'BLEACH_VS_NARUTO_SPLASH_WIDTH\s+\d+', f"BLEACH_VS_NARUTO_SPLASH_WIDTH  {w_new}", new_h_content)
    new_h_content = re.sub(r'BLEACH_VS_NARUTO_SPLASH_HEIGHT\s+\d+', f"BLEACH_VS_NARUTO_SPLASH_HEIGHT {h_new}", new_h_content)
    new_h_content = re.sub(r'BLEACH_VS_NARUTO_SPLASH_SIZE\s+\d+', f"BLEACH_VS_NARUTO_SPLASH_SIZE   {size_new}", new_h_content)

    with open(h_path, 'w', encoding='utf-8') as f:
        f.write(new_h_content)
        
    print(f"Updated Header file: {h_path} successfully.")
    return True

if __name__ == "__main__":
    base_dir = "."
    files_to_downsample = [
        ("Core/Src/bleach_vs_naruto_splash.c", "Core/Inc/bleach_vs_naruto_splash.h", 320, 240, "bleach_vs_naruto_splash"),
        ("Core/Src/mainmenu.c", "Core/Inc/mainmenu.h", 320, 240, "mainmenu"),
        ("Core/Src/choose_char.c", "Core/Inc/choose_char.h", 320, 240, "choose_char"),
        ("Core/Src/gameover.c", "Core/Inc/gameover.h", 320, 240, "gameover")
    ]
    for c_rel, h_rel, w, h, pref in files_to_downsample:
        c_path = os.path.join(base_dir, c_rel)
        h_path = os.path.join(base_dir, h_rel)
        if os.path.exists(c_path) and os.path.exists(h_path):
            downsample_file(c_path, h_path, w, h, pref)
        else:
            print(f"Warning: File not found: {c_path} or {h_path}")
