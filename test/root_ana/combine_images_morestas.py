import os
import sys
from PIL import Image
import numpy as np
import argparse

def combine_images_horizontal(image_paths, output_path):
    images = []
    for img_path in image_paths:
        if os.path.exists(img_path):
            img = Image.open(img_path)
            images.append(img)
        else:
            print(f"Warning: Image file not found: {img_path}")
            return None
    
    if not images:
        print("No valid images to combine")
        return None
    
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

def combine_images_grid(image_paths, output_path, grid_size=(2, 2)):
    images = []
    for img_path in image_paths:
        if os.path.exists(img_path):
            img = Image.open(img_path)
            images.append(img)
        else:
            print(f"Warning: Image file not found: {img_path}")
            return None
    
    if not images:
        print("No valid images to combine")
        return None
    
    widths, heights = zip(*(i.size for i in images))
    max_width = max(widths)
    max_height = max(heights)
    rows, cols = grid_size
    combined_image = Image.new('RGB', (max_width * cols, max_height * rows))
    for idx, img in enumerate(images):
        if idx >= rows * cols:
            break
        row = idx // cols
        col = idx % cols
        combined_image.paste(img, (col * max_width, row * max_height))
    combined_image.save(output_path)
    print(f"Grid image saved to {output_path}")
    return output_path

def main():
    parser = argparse.ArgumentParser(description='Combine deadtime images')
    parser.add_argument('--deadtime', type=float, default=5.0, help='Deadtime value (default: 5.0)')
    parser.add_argument('--circle1', type=float, default=40.0, help='First circle percentage (default: 40.0)')
    parser.add_argument('--circle2', type=float, default=20.0, help='Second circle percentage (default: 20.0)')
    args = parser.parse_args()
    
    deadtime = args.deadtime
    circle1 = args.circle1
    circle2 = args.circle2
    
    # Modified sizes as requested
    sizes = [100, 50, 25, 20]
    combined_images = []
    
    for size in sizes:
        print(f"Processing size {size}...")
        image_paths = [
            f"NoDeadtime_{size}x{size}_withCircles_{int(circle1)}_{int(circle2)}.png",
            f"Deadtime{deadtime}ns_{size}x{size}_withCircles.png",
            f"ReceptionRatio_Deadtime{deadtime}ns_{size}x{size}.png",
            f"RatioDistribution_Deadtime{deadtime}ns_{size}x{size}.png"
        ]
        
        output_path = f"Combined_Deadtime{deadtime}ns_{size}x{size}.png"
        combined_path = combine_images_horizontal(image_paths, output_path)
        if combined_path:
            combined_images.append(combined_path)
    
    if len(combined_images) >= 2:
        large_sizes = combined_images[:2]  # First two sizes (100, 50)
        combine_images_grid(large_sizes, f"Combined_Large_Sizes_Deadtime{deadtime}ns.png", (2, 1))
    
    if len(combined_images) >= 4:
        small_sizes = combined_images[2:4]  # Last two sizes (25, 20)
        combine_images_grid(small_sizes, f"Combined_Small_Sizes_Deadtime{deadtime}ns.png", (2, 1))
    
    print("All operations completed!")

if __name__ == "__main__":
    main()
