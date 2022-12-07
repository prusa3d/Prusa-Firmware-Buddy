import argparse
import os
import json
from pathlib import Path
import dataclasses
from PIL import Image


class ImageInfo:
    name: str
    offset: int
    size: int
    height: int
    width: int

    def __init__(self, name: str, offset: int, size: int, height: int,
                 width: int) -> None:
        self.name = name.split('.')[0]
        self.offset = offset
        self.width = width
        self.height = height
        self.size = size

    def __str__(self) -> str:
        return \
            f"""inline constexpr Resource {self.name}( \"{self.name}\", {self.offset}, {self.size}, {self.width}, {self.height});"""


def load_images(paths):
    images = []
    offset = 0
    for path in paths:
        size = os.stat(path).st_size
        with Image.open(path.resolve()) as img:
            images.append(
                ImageInfo(path.name, offset, size, img.height, img.width))
            offset = offset + size
    return images


def create_id_file(path: Path, images):
    with open(path, "w") as file:
        for image in images:
            file.write(f"{image.name},\n")


def create_resource_file(path: Path, images):
    with open(path, "w") as file:
        for image in images:
            file.write(f"{str(image)}\n")


def create_data_file(path_to_file: Path, images):
    with open(path_to_file, "wb+") as dest_file:
        for image in images:
            with open(image, "rb") as img_file:
                dest_file.write(img_file.read())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('input', type=Path, help="path to folder with pngs")
    parser.add_argument('resources',
                        type=Path,
                        help="Path to file where to store info about data")
    parser.add_argument('data_file',
                        type=Path,
                        help="Path to file where to create png file")
    images = []
    args = parser.parse_args()
    for filename in os.listdir(args.input.resolve()):
        if filename.endswith(".png"):
            images.append((args.input.resolve() / filename).resolve())
    imageInfo = load_images(images)
    create_resource_file(args.resources, imageInfo)
    create_data_file(args.data_file, images)


if __name__ == "__main__":
    main()
