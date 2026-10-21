#include "ui/chat/glass_material.h"

#include <cstdlib>
#include <iostream>
#include <QtCore/QElapsedTimer>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtGui/QPaintEvent>

namespace {

class Pattern final : public QWidget {
public:
	using QWidget::QWidget;
	QRect painted;
	bool capturing = false;
	bool fill = true;
	QColor background = Qt::red;

private:
	void paintEvent(QPaintEvent *event) override;

};

void Pattern::paintEvent(QPaintEvent *event) {
	painted = event->rect();
	capturing = Ui::Glass::CapturingBackdrop();
	auto p = QPainter(this);
	if (fill) {
		p.fillRect(event->rect(), background);
	}
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

int Benchmark() {
	auto pattern = Pattern();
	pattern.resize(960, 10000);
	auto cases = QJsonArray();
	for (const auto dpr : { 1., 2. }) {
		for (const auto size : { QSize(960, 70), QSize(960, 80), QSize(960, 62), QSize(320, 760) }) {
			auto surface = Ui::Glass::Surface();
			auto captured = QRect();
			const auto source = [&](QPainter &p, QRect clip) {
				captured = clip;
				Ui::Glass::RenderRegion(p, &pattern, QPoint(0, 4000), clip);
			};
			const auto rect = QRect(QPoint(), size);
			surface.prepare(rect, dpr, source);
			auto timer = QElapsedTimer();
			timer.start();
			for (auto i = 0; i != 80; ++i) {
				pattern.background = QColor(i * 3 % 255, i * 7 % 255, i * 11 % 255);
				surface.prepare(rect, dpr, source);
			}
			const auto changed = timer.nsecsElapsed() / 80000000.;
			timer.restart();
			for (auto i = 0; i != 80; ++i) surface.prepare(rect, dpr, source);
			const auto unchanged = timer.nsecsElapsed() / 80000000.;
			cases.append(QJsonObject{
				{ "width", size.width() }, { "height", size.height() }, { "dpr", dpr },
				{ "expanded_width", captured.width() }, { "expanded_height", captured.height() },
				{ "changed_ms", changed }, { "unchanged_ms", unchanged },
			});
		}
	}
	std::cout << QJsonDocument(QJsonObject{
		{ "kind", "synthetic QWidget capture and strip filter; excludes Telegram message painting and composition" },
		{ "qt", QT_VERSION_STR }, { "iterations", 80 }, { "cases", cases },
	}).toJson(QJsonDocument::Compact).constData() << '\n';
	return 0;
}

} // namespace

int main(int argc, char **argv) {
	qputenv("QT_QPA_PLATFORM", "offscreen");
	auto app = QApplication(argc, argv);
	if (app.arguments().contains("--benchmark")) return Benchmark();
	auto root = QWidget();
	auto left = QWidget(&root);
	auto right = QWidget(&root);
	left.move(30, 40);
	right.move(100, 120);
	Check(Ui::Glass::MapToWidget(&right, &left) == QPoint(70, 80),
		"Sibling backdrop widgets mapped to the wrong origin.");
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
	Check(pattern.capturing, "Widget paint did not identify the backdrop capture.");
	Check(!Ui::Glass::CapturingBackdrop(), "Backdrop capture leaked into normal painting.");
	auto transparent = Pattern(&root);
	transparent.resize(640, 10000);
	transparent.fill = false;
	region.fill(Qt::transparent);
	{
		auto p = QPainter(&region);
		Ui::Glass::RenderRegion(p, &transparent, QPoint(90, 4000), region.rect());
	}
	Check(qAlpha(region.pixel(0, 20)) == 0,
		"A transparent content source was flattened to its window palette.");
	Check(region.pixel(15, 20) == qRgb(0, 255, 0),
		"Transparent content capture lost foreground messages.");
	for (const auto dpr : { 1., 1.25, 1.5, 2., 3. }) {
		auto feedback = QImage(QSize(60 * dpr, 60 * dpr),
			QImage::Format_ARGB32_Premultiplied);
		feedback.setDevicePixelRatio(dpr);
		feedback.fill(Qt::transparent);
		const auto paintFeedback = [&](qreal hover, qreal pressed) {
			auto p = QPainter(&feedback);
			Ui::Glass::PaintButtonFeedback(p, QRect(10, 10, 40, 40),
				Qt::blue, hover, pressed);
			Check(p.opacity() == 1. && p.transform().isIdentity(),
				"Button feedback leaked painter state.");
		};
		const auto feedbackCenter = QPoint(30 * dpr, 30 * dpr);
		paintFeedback(0., 0.);
		Check(qAlpha(feedback.pixel(feedbackCenter)) == 0,
			"An idle glass button painted feedback.");
		paintFeedback(1., 0.);
		const auto hoverAlpha = qAlpha(feedback.pixel(feedbackCenter));
		Check(hoverAlpha > 0, "Hover feedback was invisible.");
		feedback.fill(Qt::transparent);
		paintFeedback(1., 1.);
		Check(qAlpha(feedback.pixel(feedbackCenter)) > hoverAlpha,
			"Pressed feedback did not deepen the glass material.");
		Check(qAlpha(feedback.pixel(10 * dpr, 10 * dpr)) == 0,
			"Glass button feedback painted outside its circular outline.");
		for (const auto tint : { QColor(245, 245, 245), QColor(28, 30, 33) }) {
			auto surface = Ui::Glass::Surface();
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
			Check(red.pixel(QPoint(60 * dpr, 12 * dpr)) == red.pixel(center),
				"Uniform backdrop/tint faded or changed near the capsule edge.");
			surface.clear();
			const auto fallback = Paint(surface, dpr, tint);
			Check(fallback.pixel(center) == tint.rgba(), "Opaque fallback changed the flat palette tint.");
			Check(fallback.pixel(QPoint(60 * dpr, 12 * dpr)) == tint.rgba(),
				"The capsule edge has a gradient or highlight.");
			Check(surface.checksum() == 0, "Disabled effects retained a backdrop.");
		}
	}
	auto edge = Ui::Glass::Surface();
	edge.setTintOpacity(0.);
	auto captured = QRect();
	edge.prepare(QRect(10, 10, 100, 40), 1., [&](QPainter &p, QRect clip) {
		captured = clip;
		p.fillRect(clip, Qt::white);
		p.fillRect(QRect(-100, -100, 400, 110), Qt::black);
	});
	const auto edgeImage = Paint(edge, 1., Qt::white);
	Check(captured.top() < 0 && captured.left() < 0,
		"The blur did not request source pixels outside the capsule.");
	Check(qRed(edgeImage.pixel(60, 12)) < qRed(edgeImage.pixel(60, 30)),
		"The capsule edge ignored pixels outside its bounds.");
	std::cout << "Glass material: backdrop diffusion, light/dark tint, clipping, fallback and five DPRs passed.\n";
}
