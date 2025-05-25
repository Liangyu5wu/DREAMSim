import os
import sys
from PIL import Image
import numpy as np
import argparse

def combine_images_horizontal(image_paths, output_path):
    images = [Image.open(img_path) for img_path in image_paths]
    widths, heights = zip(*(i.size for i in images))
    total_width = sum(widths)
    max_height = max(heights)
    combined_image = Image.new('RGB', (total_width, max_height))
    x_offset = 0
    for img in images:
        combined_image.paste(img, (x_offset, 0))
        x_offset += img.size[0]
    combined_image.save(output_path)
    print(f"Combined image saved to {output_path}")
    return output_path

def combine_images_grid(image_paths, output_path, grid_size=(3, 3)):
    images = [Image.open(img_path) for img_path in image_paths]
    widths, heights = zip(*(i.size for i in images))
    max_width = max(widths)
    max_height = max(heights)
    rows, cols = grid_size
    combined_image = Image.new('RGB', (max_width * cols, max_height * rows))
    for idx, img in enumerate(images):
        row = idx // cols
        col = idx % cols
        combined_image.paste(img, (col * max_width, row * max_height))
    combined_image.save(output_path)
    print(f"Grid image saved to {output_path}")
    return output_path

def main():
    parser = argparse.ArgumentParser(description='Combine deadtime images')
    parser.add_argument('--deadtime', type=float, default=30.0, help='Deadtime value (default: 30.0)')
    args = parser.parse_args()
    
    deadtime = args.deadtime
    sizes = [100, 80, 50, 40, 25, 20]
    combined_images = []
    
    for size in sizes:
        print(f"Processing size {size}...")
        image_paths = [
            f"NoDeadtime_{size}x{size}.png",
            f"Deadtime{deadtime}ns_{size}x{size}.png",
            f"ReceptionRatio_Deadtime{deadtime}ns_{size}x{size}.png"
        ]
        
        missing_files = [path for path in image_paths if not os.path.exists(path)]
        if missing_files:
            print(f"Warning: The following files are missing for size {size}: {missing_files}")
            continue
        
        output_path = f"Combined_Deadtime{deadtime}ns_{size}x{size}.png"
        combined_path = combine_images_horizontal(image_paths, output_path)
        combined_images.append(combined_path)
    
    if len(combined_images) >= 3:
        large_sizes = combined_images[:3]
        combine_images_grid(large_sizes, f"Combined_Large_Sizes_Deadtime{deadtime}ns.png", (3, 1))
    
    if len(combined_images) >= 6:
        small_sizes = combined_images[3:6]
        combine_images_grid(small_sizes, f"Combined_Small_Sizes_Deadtime{deadtime}ns.png", (3, 1))
    
    print("All operations completed!")

if __name__ == "__main__":
    main()
