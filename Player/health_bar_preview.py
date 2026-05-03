import cv2
import numpy as np

def draw_textured_rect(bg, atlas, uv, pos, size):
    u1, v1, u2, v2 = uv
    x, y, w, h = pos
    
    # Slice atlas
    ih, iw = atlas.shape[:2]
    slice_img = atlas[int(v1*ih):int(v2*ih), int(u1*iw):int(u2*iw)]
    
    # Resize to display size
    slice_resized = cv2.resize(slice_img, (int(size[0]), int(size[1])), interpolation=cv2.INTER_NEAREST)
    
    # Alpha blending
    for c in range(0, 3):
        bg[y:y+slice_resized.shape[0], x:x+slice_resized.shape[1], c] = \
            slice_resized[:,:,c] * (slice_resized[:,:,3]/255.0) + \
            bg[y:y+slice_resized.shape[0], x:x+slice_resized.shape[1], c] * (1.0 - slice_resized[:,:,3]/255.0)

def main():
    # Load assets
    atlas = cv2.imread('GUI/health_bar.png', cv2.IMREAD_UNCHANGED)
    if atlas is None:
        print("Error: Could not find GUI/health_bar.png")
        return

    # Create mock background (1920x1080)
    bg_orig = np.zeros((720, 1280, 3), dtype=np.uint8) + 50 # Dark grey background
    
    cv2.namedWindow('Dungeon Health Bar Preview')
    cv2.createTrackbar('Health', 'Dungeon Health Bar Preview', 200, 200, lambda x: None)
    cv2.createTrackbar('Scale', 'Dungeon Health Bar Preview', 100, 200, lambda x: None)

    while True:
        bg = bg_orig.copy()
        
        hp = cv2.getTrackbarPos('Health', 'Dungeon Health Bar Preview') / 10.0
        scale = cv2.getTrackbarPos('Scale', 'Dungeon Health Bar Preview') / 100.0
        
        # Current C++ Proportions
        bar_w = 120 * scale
        bar_h = 400 * scale
        
        # Center of screen
        cx, cy = 640, 360
        
        # 1. Draw Frame (UV: 0.11, 0.1, 0.5, 0.84)
        draw_textured_rect(bg, atlas, (0.11, 0.1, 0.5, 0.84), 
                          (int(cx - bar_w/2), int(cy - bar_h/2), int(bar_w), int(bar_h)), 
                          (bar_w, bar_h))
        
        # 2. Draw 7 Hearts
        total_hearts = 7
        hp_per_heart = 20.0 / total_hearts
        h_size = bar_w * 0.375
        spacing = bar_h * 0.1125
        start_y = (cy + bar_h/2) - (bar_h * 0.2) # From bottom
        
        for i in range(total_hearts):
            heart_index = total_hearts - 1 - i  # invert: top heart is depleted first
            segment_start = heart_index * hp_per_heart
            segment_end = (heart_index + 1) * hp_per_heart
            
            # UV Logic (Full, Broken, Stone)
            u1, u2 = 0.62, 0.92
            v1, v2 = 0, 0
            
            if hp >= segment_end:
                v1, v2 = 0.26, 0.39 # Full
            elif hp >= (segment_start + (hp_per_heart * 0.4)):
                v1, v2 = 0.42, 0.55 # Broken
            else:
                v1, v2 = 0.58, 0.71 # Stone
                
            heart_y = int(start_y - (i * spacing) - h_size/2)
            draw_textured_rect(bg, atlas, (u1, v1, u2, v2),
                              (int(cx - h_size/2), heart_y, int(h_size), int(h_size)),
                              (h_size, h_size))

        cv2.imshow('Dungeon Health Bar Preview', bg)
        if cv2.waitKey(1) & 0xFF == 27: # ESC to exit
            break

    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
