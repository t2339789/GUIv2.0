import os
from PIL import Image

# The bounding boxes we calculated. 
# Format for box_2d is [ymin, xmin, ymax, xmax]
sprite_data = [
    {"box_2d": [14, 132, 528, 323], "label": "frame_R1_C1"},
    {"box_2d": [131, 373, 212, 462], "label": "heart_R1_C1_top"},
    {"box_2d": [233, 373, 320, 464], "label": "heart_R1_C1_mid"},
    {"box_2d": [338, 372, 418, 464], "label": "heart_R1_C1_bot"},
    
    {"box_2d": [13, 620, 528, 811], "label": "frame_R1_C2"},
    {"box_2d": [131, 847, 212, 936], "label": "heart_R1_C2_top"},
    {"box_2d": [233, 847, 319, 938], "label": "heart_R1_C2_mid"},
    {"box_2d": [338, 847, 418, 938], "label": "heart_R1_C2_bot"},
    
    {"box_2d": [13, 1061, 528, 1252], "label": "frame_R1_C3"},
    {"box_2d": [128, 1296, 219, 1399], "label": "heart_R1_C3_top"},
    {"box_2d": [233, 1300, 319, 1395], "label": "heart_R1_C3_mid"},
    {"box_2d": [339, 1301, 418, 1394], "label": "heart_R1_C3_bot"},
    
    {"box_2d": [680, 132, 1194, 323], "label": "frame_R2_C1"},
    {"box_2d": [793, 371, 877, 464], "label": "heart_R2_C1_top"},
    {"box_2d": [893, 372, 977, 463], "label": "heart_R2_C1_mid"},
    {"box_2d": [996, 372, 1075, 463], "label": "heart_R2_C1_bot"},
    
    {"box_2d": [680, 620, 1194, 811], "label": "frame_R2_C2"},
    {"box_2d": [792, 844, 878, 939], "label": "heart_R2_C2_top"},
    {"box_2d": [893, 847, 977, 937], "label": "heart_R2_C2_mid"},
    {"box_2d": [996, 847, 1075, 937], "label": "heart_R2_C2_bot"},
    
    {"box_2d": [680, 1061, 1194, 1252], "label": "frame_R2_C3"},
    {"box_2d": [789, 1293, 877, 1387], "label": "heart_R2_C3_top"},
    {"box_2d": [893, 1296, 977, 1385], "label": "heart_R2_C3_mid"},
    {"box_2d": [996, 1296, 1074, 1385], "label": "heart_R2_C3_bot"}
]

def crop_sprites(image_path, output_dir):
    # Ensure the output directory exists
    os.makedirs(output_dir, exist_ok=True)
    
    try:
        # Load the atlas image
        img = Image.open(image_path)
    except FileNotFoundError:
        print(f"Error: Could not find '{image_path}'. Make sure the file exists in the current directory.")
        return

    print(f"Successfully loaded '{image_path}'. Cropping {len(sprite_data)} sprites...")

    for item in sprite_data:
        # Extract coordinates and label
        ymin, xmin, ymax, xmax = item["box_2d"]
        label = item["label"]
        
        # PIL's crop function expects a tuple of (left, upper, right, lower) bounds
        # which translates to (xmin, ymin, xmax, ymax)
        crop_box = (xmin, ymin, xmax, ymax)
        
        # Crop the image
        cropped_img = img.crop(crop_box)
        
        # Save the cropped image using its label as the filename
        output_filename = os.path.join(output_dir, f"{label}.png")
        cropped_img.save(output_filename)
        
        print(f"Saved: {output_filename}")

    print("\nAll sprites have been successfully cropped and saved!")

if __name__ == "__main__":
    # Define your input file and where you want the sprites saved
    input_file = "health_bar_animation.png"
    output_folder = "sprites"
    
    crop_sprites(input_file, output_folder)