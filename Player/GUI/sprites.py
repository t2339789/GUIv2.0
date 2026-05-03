import os
from PIL import Image

# The bounding boxes we calculated. 
# Format for box_2d is [ymin, xmin, ymax, xmax]
sprite_data = [
  {"box_2d":[11, 129, 531, 325], "label": "frame_R1_C1"},
  {"box_2d":[128, 371, 214, 465], "label": "heart_R1_C1_top"},
  {"box_2d":[231, 370, 322, 467], "label": "heart_R1_C1_mid"},
  {"box_2d":[337, 370, 421, 466], "label": "heart_R1_C1_bot"},
  
  {"box_2d":[11, 617, 531, 814], "label": "frame_R1_C2"},
  {"box_2d":[128, 845, 214, 939], "label": "heart_R1_C2_top"},
  {"box_2d":[231, 844, 322, 940], "label": "heart_R1_C2_mid"},
  {"box_2d": [337, 844, 421, 940], "label": "heart_R1_C2_bot"},
  
  {"box_2d":[11, 1057, 531, 1254], "label": "frame_R1_C3"},
  {"box_2d":[127, 1294, 221, 1400], "label": "heart_R1_C3_top"},
  {"box_2d":[231, 1298, 322, 1397], "label": "heart_R1_C3_mid"},
  {"box_2d":[337, 1298, 421, 1397], "label": "heart_R1_C3_bot"},
  
  {"box_2d":[678, 128, 1185, 325], "label": "frame_R2_C1"},
  {"box_2d":[790, 368, 880, 467], "label": "heart_R2_C1_top"},
  {"box_2d":[890, 370, 980, 466], "label": "heart_R2_C1_mid"},
  {"box_2d":[993, 370, 1077, 466], "label": "heart_R2_C1_bot"},
  
  {"box_2d":[678, 617, 1185, 814], "label": "frame_R2_C2"},
  {"box_2d": [789, 841, 881, 943], "label": "heart_R2_C2_top"},
  {"box_2d":[890, 844, 980, 940], "label": "heart_R2_C2_mid"},
  {"box_2d":[993, 844, 1077, 940], "label": "heart_R2_C2_bot"},
  
  {"box_2d":[678, 1057, 1185, 1254], "label": "frame_R2_C3"},
  {"box_2d":[789, 1291, 878, 1389], "label": "heart_R2_C3_top"},
  {"box_2d": [890, 1293, 980, 1387], "label": "heart_R2_C3_mid"},
  {"box_2d":[993, 1293, 1077, 1387], "label": "heart_R2_C3_bot"}
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