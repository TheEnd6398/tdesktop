#include "ui/chat/glass_material.h"

#include <cstdlib>
#include <iostream>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtGui/QPaintEvent>

namespace {

class Pattern final : public QWidget {
public:
	QRect painted;

private:
	void paintEvent(QPaintEvent *event) override;

};

void Pattern::paintEvent(QPaintEvent *event) {
	painted = event->rect();
	auto p = QPainter(this);
	p.fillRect(event->rect(), Qt::red);
	p.fillRect(QRect(100, 4000, 20, 40), Qt::green);
	p.fillRect(QRect(120, 4000, 20, 40), Qt::blue);
}

void Check(bool condition, const char *message) {
	if (!condition) {
		std::cerr << message << '\n';
		std::exit(1);
	}
}

QImage Paint(Ui::Glass::Surface &surface, qreal dpr, QColor tint) {
	auto image = QImage(QSize(120 * dpr, 60 * dpr), QImage::Format_ARGB32_Premultiplied);
	image.setDevicePixelRatio(dpr);
	image.fill(Qt::transparent);
	auto p = QPainter(&image);
	surface.paint(p, QRect(10, 10, 100, 40), tint);
	Check(p.opacity() == 1., "Painting leaked painter opacity.");
	Check(p.transform().isIdentity(), "Painting leaked a transform.");
	return image;
}

} // namespace

int main(int argc, char **argv) {
	qputenv("QT_QPA_PLATFORM", "offscreen");
	auto app = QApplication(argc, argv);
	auto pattern = Pattern();
	pattern.resize(640, 10000);
	auto region = QImage(40, 40, QImage::Format_ARGB32_Premultiplied);
	region.fill(Qt::transparent);
	{
		auto p = QPainter(&region);
		Ui::Glass::RenderRegion(p, &pattern, QPoint(100, 4000), region.rect());
	}
	Check(region.pixel(5, 20) == qRgb(0, 255, 0), "Backdrop origin was shifted.");
	Check(region.pixel(35, 20) == qRgb(0, 0, 255), "Backdrop crop was shifted.");
	Check(pattern.painted == QRect(100, 4000, 40, 40), "Backdrop rendered the full source widget.");
	for (const auto dpr : { 1., 1.25, 1.5, 2., 3. }) {
		for (const auto tint : { QColor(245, 245, 245), QColor(28, 30, 33) }) {
			auto surface = Ui::Glass::Surface();
			surface.setShadow(4);
			const auto source = [](QPainter &p, QRect clip) {
				p.fillRect(clip, Qt::red);
			};
			surface.prepare(QRect(0, 0, 120, 60), dpr, source);
			const auto redChecksum = surface.checksum();
			const auto red = Paint(surface, dpr, tint);
			surface.prepare(QRect(0, 0, 120, 60), dpr, source);
			Check(redChecksum == surface.checksum(), "Unchanged backdrop changed its checksum.");
			surface.prepare(QRect(0, 0, 120, 60), dpr, [](QPainter &p, QRect clip) {
				p.fillRect(clip, Qt::blue);
			});
			const auto blue = Paint(surface, dpr, tint);
			const auto center = QPoint(60 * dpr, 30 * dpr);
			Check(qRed(red.pixel(center)) > qRed(blue.pixel(center)), "Backdrop did not affect the material.");
			Check(qBlue(blue.pixel(center)) > qBlue(red.pixel(center)), "Backdrop color diffusion was lost.");
			Check(qAlpha(red.pixel(0, 0)) == 0, "The surface painted a rectangular background.");
			surface.clear();
			const auto fallback = Paint(surface, dpr, tint);
			Check(qAlpha(fallback.pixel(center)) == 255, "Reduced-effects fallback is not opaque.");
			Check(surface.checksum() == 0, "Disabled effects retained a backdrop.");
		}
	}
	std::cout << "Glass material: backdrop diffusion, light/dark tint, clipping, fallback and five DPRs passed.\n";
}
