#include "ui/chat/glass_material.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QPainterPath>
#include <cstdlib>
#include <iostream>
#include <set>

namespace {

void Check(bool condition, const char *message) {
	if (!condition) {
		std::cerr << message << '\n';
		std::exit(1);
	}
}

QImage Paint(qreal dpr, QColor tint, Ui::Glass::Material material) {
	auto image = QImage(QSize(180 * dpr, 100 * dpr),
		QImage::Format_ARGB32_Premultiplied);
	image.setDevicePixelRatio(dpr);
	image.fill(Qt::transparent);
	auto p = QPainter(&image);
	p.setPen(Qt::red);
	p.translate(4., 4.);
	p.setClipRect(QRect(0, 0, 160, 80));
	const auto transform = p.transform();
	const auto clip = p.clipPath();
	Ui::Glass::PaintMaterial(p, QRectF(6., 6., 148., 68.), tint,
		QColor(110, 180, 220), 14., 1., material);
	Check(p.transform() == transform && p.clipPath() == clip
		&& p.pen().color() == Qt::red && p.opacity() == 1.,
		"Material painting leaked painter state.");
	return image;
}

} // namespace

int main(int argc, char **argv) {
	qputenv("QT_QPA_PLATFORM", "offscreen");
	auto app = QGuiApplication(argc, argv);
	using Material = Ui::Glass::Material;
	for (const auto tint : { QColor(25, 29, 39), QColor(248, 249, 252) }) {
		for (const auto dpr : { 1., 1.25, 1.5, 2., 3. }) {
			auto samples = std::set<QRgb>();
			for (const auto material : { Material::Crystal, Material::Frosted,
					Material::Smoked, Material::Soft, Material::Content }) {
				const auto image = Paint(dpr, tint, material);
				const auto center = image.pixel(80 * dpr, 40 * dpr);
				Check(qAlpha(center) == 255,
					"Opaque material lost content readability without blur.");
				Check(qAlpha(image.pixel(10 * dpr, 10 * dpr)) == 0,
					"Rounded material painted outside its silhouette.");
				Check(qAlpha(image.pixel(174 * dpr, 94 * dpr)) == 0,
					"Material painting ignored the caller's clip.");
				if (material == Material::Content) {
					Check(center == tint.rgba(),
						"Content surface changed the theme's text background.");
				}
				samples.insert(image.pixel(40 * dpr, 18 * dpr));
			}
			Check(samples.size() >= 4,
				"Material levels collapsed to a uniform translucent rectangle.");
			const auto transparent = Paint(dpr, Qt::transparent, Material::Soft);
			Check(qAlpha(transparent.pixel(80 * dpr, 40 * dpr)) == 0,
				"An invisible control left a decorative backplate.");
			auto fallback = QImage(QSize(180 * dpr, 100 * dpr),
				QImage::Format_ARGB32_Premultiplied);
			fallback.setDevicePixelRatio(dpr);
			fallback.fill(Qt::transparent);
			{
				auto surface = Ui::Glass::Surface();
				surface.setMaterial(Material::Crystal,
					QColor(110, 180, 220), 1.);
				auto p = QPainter(&fallback);
				auto translucent = tint;
				translucent.setAlpha(90);
				surface.paint(p, QRect(10, 10, 148, 68), translucent, 14);
			}
			Check(qAlpha(fallback.pixel(80 * dpr, 40 * dpr)) == 255,
				"Disabled blur left translucent text surfaces.");
		}
	}
	std::cout << "Aurora materials: five levels, two appearances, five DPRs, "
		"opaque fallback, transparent controls and painter isolation passed.\n";
}
