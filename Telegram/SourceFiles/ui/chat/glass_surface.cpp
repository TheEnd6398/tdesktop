#include "ui/chat/glass_surface.h"

#include "core/application.h"
#include "core/core_settings.h"
#include "ui/power_saving.h"
#include "window/section_widget.h"
#include "window/window_session_controller.h"
#include "mainwidget.h"
#include "ui/rp_widget.h"
#include "base/invoke_queued.h"
#include "styles/style_fork_glass.h"
#include "styles/style_widgets.h"

#include <QtCore/QPointer>
#include <QtGui/QPaintEvent>


namespace Ui::Glass {
namespace {

rpl::event_stream<bool> EnabledChanges;
auto CaptureDepth = 0;

class Backdrop final : public QObject {
public:
	Backdrop(
		not_null<RpWidget*> target,
		not_null<QWidget*> source,
		not_null<Surface*> surface);

private:
	bool eventFilter(QObject *object, QEvent *event) override;
	void schedule();
	void refresh();
	[[nodiscard]] QRect sourceRect() const;

	const not_null<RpWidget*> _target;
	const QPointer<QWidget> _source;
	const not_null<Surface*> _surface;
	bool _scheduled = false;
	rpl::lifetime _lifetime;

};

Backdrop::Backdrop(
		not_null<RpWidget*> target,
		not_null<QWidget*> source,
		not_null<Surface*> surface)
: QObject(target)
, _target(target)
, _source(source)
, _surface(surface) {
	Expects(!source->isAncestorOf(target));
	_target->installEventFilter(this);
	source->installEventFilter(this);
	rpl::merge(
		EnabledValue() | rpl::to_empty,
		PowerSaving::Changes()
	) | rpl::on_next([=] {
		if (!BlurEnabled()) {
			_surface->clear();
		}
		_target->update();
		schedule();
	}, _lifetime);
	schedule();
}

QRect Backdrop::sourceRect() const {
	const auto support = st::glassBlurSupport;
	const auto bounds = _target->rect().marginsAdded(
		QMargins(support, support, support, support));
	return bounds.translated(_target->mapTo(_source, QPoint()));
}

bool Backdrop::eventFilter(QObject *object, QEvent *event) {
	if (CaptureDepth || !_source) {
		return false;
	}
	switch (event->type()) {
	case QEvent::Paint:
		if (object == _source
			&& static_cast<QPaintEvent*>(event)->rect().intersects(sourceRect())) {
			schedule();
		}
		break;
	case QEvent::Move:
	case QEvent::Resize:
	case QEvent::Show:
	case QEvent::ScreenChangeInternal:
		schedule();
		break;
	default:
		break;
	}
	return false;
}

void Backdrop::schedule() {
	if (_scheduled || _target->isHidden() || !_source || !BlurEnabled()) {
		return;
	}
	_scheduled = true;
	InvokeQueued(this, [=] { refresh(); });
}

void Backdrop::refresh() {
	_scheduled = false;
	if (!_source || _target->isHidden() || _source->isHidden()
		|| !BlurEnabled()) {
		return;
	}
	const auto before = _surface->checksum();
	const auto offset = _target->mapTo(_source, QPoint());
	const auto region = sourceRect().intersected(_source->rect());
	if (region.isEmpty()) {
		return;
	}
	++CaptureDepth;
	const auto guard = gsl::finally([] { --CaptureDepth; });
	_surface->setShadow(st::glassShadow);
	_surface->prepare(sourceRect().translated(-offset),
		_target->devicePixelRatioF(), [&](QPainter &p, QRect clip) {
			RenderRegion(p, _source, offset, clip);
		});
	if (_surface->checksum() != before) {
		_target->update();
	}
}

} // namespace

std::unique_ptr<QObject> ObserveBackdrop(
		not_null<RpWidget*> target,
		not_null<QWidget*> source,
		not_null<Surface*> surface) {
	return std::make_unique<Backdrop>(target, source, surface);
}

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
	surface.setShadow(st::glassShadow);
	if (!BlurEnabled()) {
		surface.clear();
		return;
	}
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

} // namespace Ui::Glass
