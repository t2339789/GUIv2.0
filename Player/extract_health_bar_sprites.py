import argparse
import json
from pathlib import Path

from PIL import Image


BOXES_JSON = [
    {"box_2d": [11, 129, 531, 325], "label": "frame_R1_C1"},
    {"box_2d": [128, 371, 214, 465], "label": "heart_R1_C1_top"},
    {"box_2d": [231, 370, 322, 467], "label": "heart_R1_C1_mid"},
    {"box_2d": [337, 370, 421, 466], "label": "heart_R1_C1_bot"},
    {"box_2d": [11, 617, 531, 814], "label": "frame_R1_C2"},
    {"box_2d": [128, 845, 214, 939], "label": "heart_R1_C2_top"},
    {"box_2d": [231, 844, 322, 940], "label": "heart_R1_C2_mid"},
    {"box_2d": [337, 844, 421, 940], "label": "heart_R1_C2_bot"},
    {"box_2d": [11, 1057, 531, 1254], "label": "frame_R1_C3"},
    {"box_2d": [127, 1294, 221, 1400], "label": "heart_R1_C3_top"},
    {"box_2d": [231, 1298, 322, 1397], "label": "heart_R1_C3_mid"},
    {"box_2d": [337, 1298, 421, 1397], "label": "heart_R1_C3_bot"},
    {"box_2d": [678, 128, 1185, 325], "label": "frame_R2_C1"},
    {"box_2d": [790, 368, 880, 467], "label": "heart_R2_C1_top"},
    {"box_2d": [890, 370, 980, 466], "label": "heart_R2_C1_mid"},
    {"box_2d": [993, 370, 1077, 466], "label": "heart_R2_C1_bot"},
    {"box_2d": [678, 617, 1185, 814], "label": "frame_R2_C2"},
    {"box_2d": [789, 841, 881, 943], "label": "heart_R2_C2_top"},
    {"box_2d": [890, 844, 980, 940], "label": "heart_R2_C2_mid"},
    {"box_2d": [993, 844, 1077, 940], "label": "heart_R2_C2_bot"},
    {"box_2d": [678, 1057, 1185, 1254], "label": "frame_R2_C3"},
    {"box_2d": [789, 1291, 878, 1389], "label": "heart_R2_C3_top"},
    {"box_2d": [890, 1293, 980, 1387], "label": "heart_R2_C3_mid"},
    {"box_2d": [993, 1293, 1077, 1387], "label": "heart_R2_C3_bot"},
]


def safe_crop(image: Image.Image, box: list[int]) -> tuple[Image.Image, list[int], bool]:
    x1, y1, x2, y2 = box
    if not (x1 < x2 and y1 < y2):
        raise ValueError(f"Invalid box coordinates: {box}")

    cx1 = max(0, min(x1, image.width))
    cy1 = max(0, min(y1, image.height))
    cx2 = max(0, min(x2, image.width))
    cy2 = max(0, min(y2, image.height))

    if not (cx1 < cx2 and cy1 < cy2):
        raise ValueError(
            f"Box {box} has no overlap with atlas bounds {image.width}x{image.height}"
        )

    clipped = [cx1, cy1, cx2, cy2] != [x1, y1, x2, y2]
    return image.crop((cx1, cy1, cx2, cy2)), [cx1, cy1, cx2, cy2], clipped


def export_group_sheet(
    output_dir: Path, sprites: dict[str, Image.Image], row: int, col: int
) -> None:
    frame_key = f"frame_R{row}_C{col}"
    top_key = f"heart_R{row}_C{col}_top"
    mid_key = f"heart_R{row}_C{col}_mid"
    bot_key = f"heart_R{row}_C{col}_bot"

    frame_img = sprites[frame_key]
    heart_imgs = [sprites[top_key], sprites[mid_key], sprites[bot_key]]

    gap = 10
    hearts_height = sum(img.height for img in heart_imgs) + (gap * (len(heart_imgs) - 1))
    sheet_w = frame_img.width + gap + max(img.width for img in heart_imgs)
    sheet_h = max(frame_img.height, hearts_height)

    sheet = Image.new("RGBA", (sheet_w, sheet_h), (0, 0, 0, 0))
    sheet.alpha_composite(frame_img, (0, 0))

    y = 0
    for img in heart_imgs:
        sheet.alpha_composite(img, (frame_img.width + gap, y))
        y += img.height + gap

    sheet.save(output_dir / f"group_R{row}_C{col}.png")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Extract health bar frame/heart sprites from atlas PNG."
    )
    parser.add_argument(
        "--atlas",
        default="GUI/health_bar_animation.png",
        help="Path to source atlas PNG",
    )
    parser.add_argument(
        "--out",
        default="GUI/extracted_health_bar_sprites",
        help="Directory where extracted PNGs will be written",
    )
    args = parser.parse_args()

    atlas_path = Path(args.atlas)
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    atlas = Image.open(atlas_path).convert("RGBA")

    sprites: dict[str, Image.Image] = {}
    manifest: dict[str, dict] = {}

    for entry in BOXES_JSON:
        label = entry["label"]
        box = entry["box_2d"]
        sprite, final_box, was_clipped = safe_crop(atlas, box)
        sprite_path = out_dir / f"{label}.png"
        sprite.save(sprite_path)

        sprites[label] = sprite
        manifest[label] = {
            "box_2d_requested": box,
            "box_2d_used": final_box,
            "clipped_to_atlas_bounds": was_clipped,
            "width": sprite.width,
            "height": sprite.height,
            "file": sprite_path.name,
        }

    for row in (1, 2):
        for col in (1, 2, 3):
            export_group_sheet(out_dir, sprites, row, col)

    with (out_dir / "manifest.json").open("w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)

    print(f"Extracted {len(BOXES_JSON)} sprites to: {out_dir}")
    print(f"Wrote grouped previews and manifest: {out_dir / 'manifest.json'}")


if __name__ == "__main__":
    main()
