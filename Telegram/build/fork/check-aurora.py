import json
import re
from pathlib import Path
from zipfile import ZipFile


def palette(text):
    return dict(re.findall(r"^(\w+):\s*([^;]+);", text, re.M))


def resolve(values, key, seen=()):
    if key in seen:
        raise AssertionError(f"Palette cycle: {key}")
    value = values[key].split("|")[0].strip()
    if not value.startswith("#"):
        return resolve(values, value, (*seen, key))
    return tuple(int(value[i:i + 2], 16) / 255 for i in (1, 3, 5))


def luminance(rgb):
    def linear(value):
        return value / 12.92 if value <= 0.04045 else ((value + 0.055) / 1.055) ** 2.4
    return sum(linear(c) * w for c, w in zip(rgb, (0.2126, 0.7152, 0.0722)))


def main():
    telegram = Path(__file__).resolve().parents[2]
    source = json.loads((telegram / "Resources/aurora/palettes.json").read_text())
    base = palette((telegram / "lib_ui/ui/colors.palette").read_text())
    with ZipFile(telegram / "Resources/night.tdesktop-theme") as archive:
        dark = base | palette(archive.read("colors.tdesktop-theme").decode("utf-8"))
    pairs = (
        ("windowFg", "windowBg"),
        ("windowSubTextFg", "windowBg"),
        ("windowActiveTextFg", "windowBg"),
        ("dialogsNameFg", "dialogsBg"),
        ("dialogsTextFg", "dialogsBg"),
        ("dialogsDateFg", "dialogsBg"),
        ("dialogsNameFgActive", "dialogsBgActive"),
        ("dialogsTextFgActive", "dialogsBgActive"),
        ("dialogsDateFgActive", "dialogsBgActive"),
        ("dialogsUnreadFg", "dialogsUnreadBg"),
        ("dialogsUnreadFg", "dialogsUnreadBgMuted"),
        ("dialogsUnreadFgActive", "dialogsUnreadBgActive"),
        ("dialogsUnreadFgActive", "dialogsUnreadBgMutedActive"),
        ("historyTextInFg", "msgInBg"),
        ("historyTextOutFg", "msgOutBg"),
        ("msgInDateFg", "msgInBg"),
        ("msgOutDateFg", "msgOutBg"),
    )
    for mode, values in (("light", base), ("dark", dark)):
        for key, expected in source[mode].items():
            assert key in base, f"Unknown palette token: {key}"
            assert values[key] == expected, f"Stale {mode} token: {key}"
        for fg, bg in pairs:
            light, dark_value = sorted((
                luminance(resolve(values, fg)), luminance(resolve(values, bg))
            ), reverse=True)
            contrast = (light + 0.05) / (dark_value + 0.05)
            assert contrast >= 4.5, f"{mode}: {fg}/{bg} contrast is {contrast:.2f}:1"
    print(f"Aurora: palette resources and {len(pairs) * 2} text contrast pairs passed.")


if __name__ == "__main__":
    main()
