import sys
import os

try:
    from PIL import Image
except ImportError:
    print("Error: The 'Pillow' library is not installed.")
    print("Please install it by running: pip install Pillow")
    sys.exit(1)

def convert_ppm_to_png(input_path, output_path):
    if not os.path.exists(input_path):
        print(f"Error: The input file '{input_path}' does not exist.")
        return

    try:
        print(f"Opening '{input_path}'...")
        with Image.open(input_path) as img:

            img.save(output_path)

            print(f"Success! Converted to '{output_path}'")

    except Exception as e:
        print(f"An error occurred during conversion: {e}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python ppm_to_png.py <input_file.ppm> <output_file.png>")
        print("Example: python ppm_to_png.py render.ppm render.png")
    else:
        input_file = sys.argv[1]
        output_file = sys.argv[2]
        convert_ppm_to_png(input_file, output_file)