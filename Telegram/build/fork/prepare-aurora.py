import argparse
import json
import re
import struct
import zlib
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo


def update_colors(text, colors):
    for key, value in colors.items():
        pattern = re.compile(r"^(" + re.escape(key) + r":\s*)[^;]+;", re.M)
        if pattern.search(text):
            text = pattern.sub(lambda match: match[1] + value + ";", text)
        else:
            text += f"\n{key}: {value};\n"
    return text


def png(color):
    def chunk(kind, data):
        return struct.pack("!I", len(data)) + kind + data + struct.pack(
            "!I", zlib.crc32(kind + data)
        )

    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack("!IIBBBBB", 1, 1, 8, 2, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(b"\x00" + bytes(color)))
        + chunk(b"IEND", b"")
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    telegram = Path(__file__).resolve().parents[2]
    palettes = json.loads((telegram / "Resources/aurora/palettes.json").read_text())
    palette_path = telegram / "lib_ui/ui/colors.palette"
    palette = palette_path.read_text()
    light = update_colors(palette, palettes["light"])
    theme_path = telegram / "Resources/night.tdesktop-theme"
    with ZipFile(theme_path) as archive:
        original = archive.read("colors.tdesktop-theme").decode("utf-8")
        old_background = archive.read("background.png")
    dark = update_colors(original, palettes["dark"])
    background = png((18, 22, 31))
    if args.check:
        if palette != light or original != dark or old_background != background:
            raise SystemExit("Aurora palette resources need regeneration.")
        print("Aurora light and dark resources match their source palettes.")
        return
    palette_path.write_text(light)
    with ZipFile(theme_path, "w", compression=ZIP_DEFLATED) as archive:
        for name, data in (
            ("colors.tdesktop-theme", dark.encode("utf-8")),
            ("background.png", background),
        ):
            info = ZipInfo(name, (2026, 1, 1, 0, 0, 0))
            info.compress_type = ZIP_DEFLATED
            archive.writestr(info, data)


if __name__ == "__main__":
    main()
