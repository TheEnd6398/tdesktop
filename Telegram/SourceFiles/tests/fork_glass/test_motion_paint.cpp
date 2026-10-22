#include "ui/effects/motion_paint.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char *message) {
	if (!condition) {
		std::cerr << message << '\n';
		std::exit(1);
	}
}

QImage Paint(double dpr, QColor accent, double hover, double press) {
	auto image = QImage(QSize(180 * dpr, 100 * dpr), QImage::Format_ARGB32_Premultiplied);
	image.setDevicePixelRatio(dpr);
	image.fill(Qt::transparent);
	auto p = QPainter(&image);
	p.setPen(Qt::red);
	p.setOpacity(0.7);
	p.translate(4., 4.);
	p.setClipRect(QRect(0, 0, 145, 76));
	const auto transform = p.transform();
	const auto clip = p.clipPath();
	Ui::Motion::PaintInteraction(p, QRectF(6., 6., 148., 68.), 14., 1., accent, hover, press);
	Check(p.transform() == transform && p.clipPath() == clip
		&& p.pen().color() == Qt::red && p.opacity() == 0.7,
		"Motion rendering leaked painter state.");
	return image;
}

} // namespace

int main(int argc, char **argv) {
	for (const auto dpr : { 1., 1.25, 1.5, 2., 3. }) {
		for (const auto accent : { QColor(45, 120, 170), QColor(120, 200, 240) }) {
			const auto idle = Paint(dpr, accent, 0., 0.);
			const auto hover = Paint(dpr, accent, 0.5, 0.);
			const auto pressed = Paint(dpr, accent, 1., 1.);
			Check(qAlpha(idle.pixel(80 * dpr, 40 * dpr)) == 0,
				"Idle controls retained decorative illumination.");
			Check(hover != idle && pressed != hover,
				"Hover and press do not produce distinct interaction feedback.");
			Check(qAlpha(pressed.pixel(10 * dpr, 10 * dpr)) == 0
				&& qAlpha(pressed.pixel(160 * dpr, 50 * dpr)) == 0,
				"Motion painted outside the rounded control or caller's clip.");
		}
	}
	if (argc == 3 && std::string(argv[1]) == "--benchmark") {
		auto samples = std::vector<double>();
		auto timer = QElapsedTimer();
		for (auto i = 0; i != 500; ++i) {
			timer.start();
			const auto frame = Paint(2., QColor(120, 200, 240), (i % 30) / 30., (i % 11) / 11.);
			Check(!frame.isNull(), "Motion benchmark did not render.");
			samples.push_back(timer.nsecsElapsed() / 1000000.);
		}
		std::sort(samples.begin(), samples.end());
		const auto report = QJsonObject{
			{ "scope", "Qt raster interaction component, 180x100 logical pixels, DPR 2; not application frame pacing" },
			{ "samples", int(samples.size()) },
			{ "p50_ms", samples[samples.size() / 2] },
			{ "p95_ms", samples[samples.size() * 95 / 100] },
		};
		auto file = QFile(QString::fromLocal8Bit(argv[2]));
		Check(file.open(QIODevice::WriteOnly), "Cannot write the motion benchmark report.");
		file.write(QJsonDocument(report).toJson());
	}
	std::cout << "Interaction rendering: five DPRs, two accents, idle, hover, press, clipping and painter isolation passed.\n";
}
