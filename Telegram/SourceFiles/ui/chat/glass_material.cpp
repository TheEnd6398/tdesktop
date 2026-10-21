#include "ui/chat/glass_material.h"

#include "ui/chat/glass_blur.h"

#include <QtCore/QHashFunctions>
#include <QtCore/QScopedValueRollback>
#include <QtGui/QPainterPath>
#include <QtWidgets/QWidget>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace Ui::Glass {
namespace {

auto RenderDepth = 0;

} // namespace

bool CapturingBackdrop() {
	return RenderDepth > 0;
}

QPoint MapToWidget(QWidget *from, QWidget *to) {
	return to->mapFromGlobal(from->mapToGlobal(QPoint()));
}

void RenderRegion(QPainter &p, QWidget *source, QPoint offset, QRect bounds) {
	const auto region = bounds.translated(offset).intersected(source->rect());
	if (!region.isEmpty()) {
		const auto guard = QScopedValueRollback(RenderDepth, RenderDepth + 1);
		source->render(&p, region.topLeft() - offset, QRegion(region),
			QWidget::DrawChildren);
	}
}

void Surface::clear() {
	_image = _scratch = QImage();
	_source = QImage();
	_bounds = {};
	_checksum = 0;
	_sourceChecksum = 0;
	_prepared = false;
}

void Surface::prepare(QRect bounds, qreal dpr, const Source &source) {
	if (!source || bounds.isEmpty()) {
		clear();
		return;
	}
	_bounds = bounds;
	const auto scale = std::min(dpr, qreal(2.)) / 4.;
	const auto size = QSize(
		std::max(1, int(std::ceil(bounds.width() * scale))),
		std::max(1, int(std::ceil(bounds.height() * scale))));
	if (_image.size() != size) {
		_source = QImage(size, QImage::Format_ARGB32_Premultiplied);
		_image = QImage(size, QImage::Format_ARGB32_Premultiplied);
		_scratch = QImage(size, QImage::Format_ARGB32_Premultiplied);
		_prepared = false;
	}
	_source.fill(Qt::transparent);
	{
		auto p = QPainter(&_source);
		p.scale(scale, scale);
		p.translate(-bounds.topLeft());
		source(p, bounds);
	}
	const auto checksum = qHashBits(_source.constBits(), _source.sizeInBytes(),
		quint64(_source.width()) * 65537 + _source.height());
	const auto radius = std::max(1, int(std::round(3 * std::min(dpr, qreal(2.)))));
	if (_prepared && _sourceChecksum == checksum && _radius == radius) {
		return;
	}
	_sourceChecksum = checksum;
	_prepared = true;
	_radius = radius;
	std::memcpy(_image.bits(), _source.constBits(), _source.sizeInBytes());
	Blur(_image, _scratch, radius);
	_checksum = qHashBits(_image.constBits(), _image.sizeInBytes(),
		quint64(_image.width()) * 65537 + _image.height());
}

void Surface::paint(QPainter &p, QRect rect, QColor tint, int radius) const {
	if (rect.isEmpty()) {
		return;
	}
	p.save();
	p.setRenderHint(QPainter::Antialiasing);
	const auto rounded = QRectF(rect);
	const auto r = radius ? radius : rect.height() / 2.;
	p.setPen(Qt::NoPen);

	auto path = QPainterPath();
	path.addRoundedRect(rounded, r, r);
	p.setClipPath(path, Qt::IntersectClip);
	if (!_image.isNull()) {
		p.setRenderHint(QPainter::SmoothPixmapTransform);
		p.drawImage(_bounds, _image);
		tint.setAlphaF(_tintOpacity);
	} else {
		tint.setAlpha(255);
	}
	p.fillPath(path, tint);
	p.restore();
}

void Surface::setTintOpacity(qreal opacity) {
	_tintOpacity = std::clamp(opacity, 0., 1.);
}

void PaintButtonFeedback(
		QPainter &p,
		QRect rect,
		QColor color,
		qreal hover,
		qreal pressed) {
	if (rect.isEmpty() || (hover <= 0. && pressed <= 0.)) {
		return;
	}
	p.save();
	p.setRenderHint(QPainter::Antialiasing);
	const auto radius = rect.height() / 2.;
	auto path = QPainterPath();
	path.addRoundedRect(QRectF(rect), radius, radius);
	p.setClipPath(path, Qt::IntersectClip);
	color.setAlphaF(std::clamp(0.08 * hover + 0.12 * pressed, 0., 1.));
	p.fillPath(path, color);
	p.restore();
}

quint64 Surface::checksum() const {
	return _checksum;
}

} // namespace Ui::Glass
