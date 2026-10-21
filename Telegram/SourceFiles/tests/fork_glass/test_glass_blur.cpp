#include "ui/chat/glass_blur.h"

#include <QtCore/QByteArray>
#include <algorithm>
#include <cstdlib>
#include <iostream>

namespace {

QImage Reference(QImage image, int radius) {
	for (auto pass = 0; pass != 3; ++pass) {
		for (auto axis = 0; axis != 2; ++axis) {
			auto output = QImage(image.size(), image.format());
			for (auto y = 0; y != image.height(); ++y) {
				for (auto x = 0; x != image.width(); ++x) {
					auto red = 0, green = 0, blue = 0, alpha = 0;
					for (auto i = -radius; i <= radius; ++i) {
						const auto pixel = image.pixel(
							std::clamp(x + (axis ? 0 : i), 0, image.width() - 1),
							std::clamp(y + (axis ? i : 0), 0, image.height() - 1));
						red += qRed(pixel);
						green += qGreen(pixel);
						blue += qBlue(pixel);
						alpha += qAlpha(pixel);
					}
					const auto samples = 2 * radius + 1;
					output.setPixel(x, y, qRgba(red / samples, green / samples, blue / samples, alpha / samples));
				}
			}
			image = std::move(output);
		}
	}
	return image;
}

void Check(bool condition, const char *message) {
	if (!condition) {
		std::cerr << message << '\n';
		std::exit(1);
	}
}

} // namespace

int main() {
	auto scratch = QImage();
	auto empty = QImage();
	Ui::Glass::Blur(empty, scratch);
	Check(scratch.isNull(), "Empty input allocated a buffer.");
	for (const auto radius : { 0, 1, 3, 4, 6 }) {
		for (const auto size : { QSize(1, 1), QSize(1, 19), QSize(23, 1), QSize(31, 17) }) {
			auto image = QImage(size, QImage::Format_ARGB32_Premultiplied);
			image.fill(qRgba(25, 50, 75, 100));
			const auto original = image.copy();
			Ui::Glass::Blur(image, scratch, radius);
			Check(image == original, "Uniform premultiplied color changed.");
			for (auto y = 0; y != size.height(); ++y) {
				for (auto x = 0; x != size.width(); ++x) {
					image.setPixel(x, y, qPremultiply(qRgba(
						(x * 17) % 256, (y * 29) % 256, (x * y * 7) % 256,
						(x * 13 + y * 41) % 256)));
				}
			}
			const auto expected = Reference(image.copy(), radius);
			const auto imageStorage = image.constBits();
			const auto scratchStorage = scratch.constBits();
			Ui::Glass::Blur(image, scratch, radius);
			Check(image == expected, "Blur differs from the reference at edges or alpha.");
			Check(image.constBits() == imageStorage, "Image storage was reallocated.");
			Check(scratch.constBits() == scratchStorage, "Scratch storage was reallocated.");
		}
	}
	auto storage = QByteArray(64 * 17, char(0x5A));
	auto padded = QImage(reinterpret_cast<uchar*>(storage.data()), 5, 17, 64,
		QImage::Format_ARGB32_Premultiplied);
	for (auto y = 0; y != 17; ++y) {
		for (auto x = 0; x != 5; ++x) padded.setPixel(x, y, qRgb(x * 40, y * 12, 80));
	}
	const auto expected = Reference(padded.copy(), 3);
	Ui::Glass::Blur(padded, scratch, 3);
	for (auto y = 0; y != 17; ++y) {
		for (auto x = 0; x != 5; ++x) {
			Check(padded.pixel(x, y) == expected.pixel(x, y), "Padded-row blur changed pixel values.");
		}
		for (auto byte = 20; byte != 64; ++byte) {
			Check(storage.at(y * 64 + byte) == char(0x5A), "Blur overwrote row padding.");
		}
	}
	std::cout << "Glass blur: empty, thin strips, alpha, edges and buffer reuse passed.\n";
}
