from PIL import Image, ImageDraw, ImageFont
import os

WIDTH = 32
HEIGHT = 48
OUTPUT_DIR = "./"

os.makedirs(OUTPUT_DIR, exist_ok=True)

try:
    font = ImageFont.truetype("DejaVuSans.ttf", 32)
except OSError:
    font = ImageFont.load_default()

for code in range(128):

    image = Image.new(
        "RGBA",
        (WIDTH, HEIGHT),
        (0, 0, 0, 0)
    )

    if 32 <= code <= 126:

        character = chr(code)

        if character.isprintable() and character not in '':
        
            draw = ImageDraw.Draw(image)

            bbox = draw.textbbox((0, 0), character, font=font)

            text_width = bbox[2] - bbox[0]
            text_height = bbox[3] - bbox[1]

            x = (WIDTH - text_width) // 2 - bbox[0]
            y = (HEIGHT - text_height) // 2 - bbox[1]

            draw.text(
                (x, y),
                character,
                font=font,
                fill=(255, 255, 255, 255)
            )

            filename = f"{code}.png"
            image.save(os.path.join(OUTPUT_DIR, filename))

print("Generated all 128 ASCII images!")
