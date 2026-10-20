#pragma once

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <functional>

class QWidget;

namespace Ui::Glass {

[[nodiscard]] QPoint MapToWidget(QWidget *from, QWidget *to);
[[nodiscard]] bool CapturingBackdrop();
void RenderRegion(QPainter &p, QWidget *source, QPoint offset, QRect bounds);
void PaintHighlight(QPainter &p, QRect rect, int radius = 0);
void PaintFocus(QPainter &p, QRect rect, QColor color, qreal opacity, int radius);
void PaintButtonFeedback(
	QPainter &p,
	QRect rect,
	QColor color,
	qreal hover,
	qreal pressed);

class Surface final {
public:
	using Source = std::function<void(QPainter &p, QRect clip)>;

	void prepare(QRect bounds, qreal dpr, const Source &source);
	void paint(QPainter &p, QRect rect, QColor tint, int radius = 0) const;
	void setShadow(int shadow);
	void clear();
	[[nodiscard]] quint64 checksum() const;

private:
	QImage _source;
	QImage _image;
	QImage _scratch;
	QRect _bounds;
	int _shadow = 0;
	quint64 _checksum = 0;
	quint64 _sourceChecksum = 0;
	bool _prepared = false;
	int _radius = 0;

};

} // namespace Ui::Glass
