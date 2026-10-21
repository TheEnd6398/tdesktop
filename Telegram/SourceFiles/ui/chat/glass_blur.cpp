#include "ui/chat/glass_blur.h"

#include <algorithm>

namespace Ui::Glass {

void Blur(QImage &image, QImage &scratch, int radius) {
	if (image.isNull()) {
		return;
	}
	if (scratch.size() != image.size()
		|| scratch.format() != image.format()) {
		scratch = QImage(image.size(), image.format());
	}
	Q_ASSERT(image.format() == QImage::Format_ARGB32_Premultiplied);
	Q_ASSERT(radius >= 0);
	const auto samples = radius * 2 + 1;
	const auto width = image.width();
	const auto height = image.height();
	for (auto pass = 0; pass != 3; ++pass) {
		for (auto axis = 0; axis != 2; ++axis) {
			const auto &from = axis ? scratch : image;
			auto &to = axis ? image : scratch;
			const auto length = axis ? height : width;
			const auto lines = axis ? width : height;
			const auto input = reinterpret_cast<const QRgb*>(from.constBits());
			const auto output = reinterpret_cast<QRgb*>(to.bits());
			const auto inputStride = from.bytesPerLine() / sizeof(QRgb);
			const auto outputStride = to.bytesPerLine() / sizeof(QRgb);
			for (auto line = 0; line != lines; ++line) {
				const auto pixel = [&](int position) {
					position = std::clamp(position, 0, length - 1);
					return input[(axis ? position : line) * inputStride + (axis ? line : position)];
				};
				auto red = 0;
				auto green = 0;
				auto blue = 0;
				auto alpha = 0;
				const auto add = [&](QRgb value, int sign) {
					red += sign * qRed(value);
					green += sign * qGreen(value);
					blue += sign * qBlue(value);
					alpha += sign * qAlpha(value);
				};
				for (auto i = -radius; i <= radius; ++i) {
					add(pixel(i), 1);
				}
				for (auto i = 0; i != length; ++i) {
					output[(axis ? i : line) * outputStride + (axis ? line : i)] = qRgba(
						red / samples,
						green / samples,
						blue / samples,
						alpha / samples);
					add(pixel(i - radius), -1);
					add(pixel(i + radius + 1), 1);
				}
			}
		}
	}
}

} // namespace Ui::Glass
