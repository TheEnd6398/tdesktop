#include "ui/chat/glass_surface.h"

#include "core/application.h"
#include "core/core_settings.h"
#include "ui/chat/glass_blur.h"
#include "ui/power_saving.h"
#include "window/section_widget.h"
#include "window/window_session_controller.h"
#include "mainwidget.h"
#include "styles/style_fork_glass.h"
#include "styles/style_widgets.h"

#include <QtGui/QPainterPath>
#include <cmath>

namespace Ui::Glass {
namespace {

rpl::event_stream<bool> EnabledChanges;

} // namespace

bool Enabled() {
#ifdef TDESKTOP_FORK_LIQUID_GLASS
	return Core::App().settings().readPref<bool>("fork/capsule-layout", true);
#else
	return false;
#endif
}

void SetEnabled(bool enabled) {
	if (Enabled() == enabled) {
		return;
	}
	Core::App().settings().writePref<bool>("fork/capsule-layout", enabled);
	Core::App().saveSettingsDelayed();
	EnabledChanges.fire_copy(enabled);
}

rpl::producer<bool> EnabledValue() {
	return rpl::single(Enabled()) | rpl::then(EnabledChanges.events());
}

bool BlurEnabled() {
	return Enabled() && !PowerSaving::On(PowerSaving::kGlassBlur);
}

QRect ButtonRect(QRect rect) {
	const auto side = std::min(rect.width(), rect.height()) - 2 * st::lineWidth;
	return QRect(rect.center() - QPoint(side / 2, side / 2), QSize(side, side));
}

const style::InputField &FieldStyle(
		const style::InputField &original,
		rpl::lifetime &lifetime) {
#ifdef TDESKTOP_FORK_LIQUID_GLASS
	const auto result = lifetime.make_state<style::InputField>(original);
	result->textBg = result->textBgActive = st::transparent;
	return *result;
#else
	return original;
#endif
}

void PrepareChat(
		Surface &surface,
		not_null<QWidget*> widget,
		not_null<Window::SessionController*> controller,
		QRect bounds) {
	const auto content = controller->content();
	const auto origin = widget->mapTo(content, QPoint());
	const auto offset = QPoint(0, origin.y() - content->backgroundFromY());
	surface.prepare(bounds.isEmpty() ? widget->rect() : bounds,
		widget->devicePixelRatioF(),
		[&](QPainter &p, QRect clip) {
			p.translate(-offset);
			Window::SectionWidget::PaintBackground(
				p,
				controller->currentChatTheme(),
				QSize(widget->width(), content->height()),
				clip.translated(offset),
				true);
		});
}

void Surface::clear() {
	_image = _scratch = QImage();
	_bounds = {};
}

void Surface::prepare(QRect bounds, qreal dpr, const Source &source) {
	if (!BlurEnabled() || !source || bounds.isEmpty()) {
		clear();
		return;
	}
	_bounds = bounds;
	const auto scale = std::min(dpr, qreal(2.)) / 4.;
	const auto size = QSize(
		std::max(1, int(std::ceil(bounds.width() * scale))),
		std::max(1, int(std::ceil(bounds.height() * scale))));
	if (_image.size() != size) {
		_image = QImage(size, QImage::Format_ARGB32_Premultiplied);
		_scratch = QImage(size, QImage::Format_ARGB32_Premultiplied);
	}
	_image.fill(Qt::transparent);
	{
		auto p = QPainter(&_image);
		p.scale(scale, scale);
		p.translate(-bounds.topLeft());
		source(p, bounds);
	}
	Blur(_image, _scratch);
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
	for (auto i = st::glassShadow; i > 0; --i) {
		p.setBrush(QColor(0, 0, 0, 3));
		p.drawRoundedRect(rounded.adjusted(-i, 0, i, i), r + i, r + i);
	}
	auto path = QPainterPath();
	path.addRoundedRect(rounded, r, r);
	p.setClipPath(path, Qt::IntersectClip);
	if (!_image.isNull() && BlurEnabled()) {
		p.setRenderHint(QPainter::SmoothPixmapTransform);
		p.drawImage(_bounds, _image);
		tint.setAlpha(190);
	} else {
		tint.setAlpha(255);
	}
	p.fillPath(path, tint);
	auto light = QLinearGradient(rounded.topLeft(), rounded.bottomLeft());
	light.setColorAt(0., QColor(255, 255, 255, 32));
	light.setColorAt(0.45, QColor(255, 255, 255, 4));
	light.setColorAt(1., QColor(0, 0, 0, 10));
	p.fillPath(path, light);
	const auto inset = 0.5 / p.device()->devicePixelRatioF();
	p.setBrush(Qt::NoBrush);
	p.setPen(QPen(QColor(255, 255, 255, 58), 2 * inset));
	p.drawRoundedRect(rounded.adjusted(inset, inset, -inset, -inset), r, r);
	p.restore();
}

} // namespace Ui::Glass
