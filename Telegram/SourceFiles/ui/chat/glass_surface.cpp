#include "ui/chat/glass_surface.h"

#include "core/application.h"
#include "core/core_settings.h"
#include "ui/power_saving.h"
#include "window/section_widget.h"
#include "window/window_session_controller.h"
#include "mainwidget.h"
#include "ui/rp_widget.h"
#include "ui/abstract_button.h"
#include "ui/widgets/elastic_scroll.h"
#include "ui/effects/animations.h"
#include "base/invoke_queued.h"
#include "styles/style_fork_glass.h"
#include "styles/style_widgets.h"

#include <QtCore/QPointer>
#include <QtCore/QCoreApplication>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainterPath>


namespace Ui::Glass {
namespace {

rpl::event_stream<bool> EnabledChanges;

constexpr auto kHoverDuration = crl::time(140);
constexpr auto kPressDuration = crl::time(90);

class ChatContent final : public RpWidget {
public:
	ChatContent(QWidget *parent, object_ptr<RpWidget> content)
	: RpWidget(parent), _content(std::move(content)) {
		_content->setParent(this);
		_content->show();
		_content->sizeValue() | rpl::on_next([=] { refresh(); }, lifetime());
	}
	void setInsets(int top, int bottom) {
		_top = top;
		_bottom = bottom;
		refresh();
	}
	[[nodiscard]] int reserve() const { return _top + _bottom; }
	[[nodiscard]] int top() const { return _top; }
	[[nodiscard]] int bottom() const { return _bottom; }
private:
	void refresh() {
		_content->move(0, _top);
		resize(_content->width(), _content->height() + reserve());
	}
	object_ptr<RpWidget> _content;
	int _top = 0;
	int _bottom = 0;
};

QWidget *ChatSource(QWidget *target) {
	for (auto widget = target; widget; widget = widget->parentWidget()) {
		const auto object = widget->property("forkChatScroll").value<QObject*>();
		if (const auto scroll = dynamic_cast<ElasticScroll*>(object)) {
			return scroll->widget();
		}
	}
	return nullptr;
}

struct ButtonFeedback {
	Ui::Animations::Simple hover;
	Ui::Animations::Simple press;
	bool over = false;
	bool down = false;
};

class Backdrop final : public QObject {
public:
	Backdrop(
		not_null<RpWidget*> target,
		not_null<QWidget*> source,
		not_null<Surface*> surface,
		Window::SessionController *controller = nullptr,
		QWidget *sampler = nullptr, Fn<QRect()> bounds = {});
	~Backdrop() override;

private:
	bool eventFilter(QObject *object, QEvent *event) override;
	void schedule();
	void refresh();

	const not_null<RpWidget*> _target;
	QPointer<QWidget> _source;
	const QPointer<QWidget> _sampler;
	Window::SessionController *_controller = nullptr;
	Fn<QRect()> _bounds;
	const not_null<Surface*> _surface;
	bool _scheduled = false;
	bool _enabled = false;
	rpl::lifetime _lifetime;

};

Backdrop::Backdrop(
		not_null<RpWidget*> target,
		not_null<QWidget*> source,
		not_null<Surface*> surface,
		Window::SessionController *controller,
		QWidget *sampler, Fn<QRect()> bounds)
: QObject(target)
, _target(target)
, _source(source)
, _sampler(sampler)
, _controller(controller)
, _bounds(std::move(bounds))
, _surface(surface) {
	Expects(controller || !source->isAncestorOf(target));
	QCoreApplication::instance()->installEventFilter(this);
	QObject::connect(source, &QObject::destroyed, this, [=] {
		_surface->clear();
		_target->update();
	});
	rpl::merge(
		EnabledValue() | rpl::to_empty,
		PowerSaving::Changes()
	) | rpl::on_next([=] {
		_enabled = BlurEnabled();
		if (!_enabled) {
			_surface->clear();
		}
		_target->update();
		schedule();
	}, _lifetime);
	schedule();
}

Backdrop::~Backdrop() {
	if (const auto app = QCoreApplication::instance()) {
		app->removeEventFilter(this);
	}
}

bool Backdrop::eventFilter(QObject *object, QEvent *event) {
	if (!CapturingBackdrop() && _enabled
		&& event->type() == QEvent::UpdateRequest && object == _target->window()) {
		refresh();
		return false;
	}
	if (CapturingBackdrop() || _scheduled || !_source || !_enabled || _target->isHidden()) {
		return false;
	}
	switch (event->type()) {
	case QEvent::Move:
	case QEvent::Resize:
	case QEvent::Show:
	case QEvent::ScreenChangeInternal:
		if (object == _target || object == _source) {
			schedule();
		}
		break;
	default:
		break;
	}
	return false;
}

void Backdrop::schedule() {
	if (_scheduled || _target->isHidden() || !_source || !_enabled
		|| !_target->isVisibleTo(_target->window()) || _target->window()->isHidden()) {
		return;
	}
	_scheduled = true;
	InvokeQueued(this, [=] { refresh(); });
}

void Backdrop::refresh() {
	_scheduled = false;
	if (_controller && _sampler) {
		_source = ChatSource(_sampler);
	}
	if (!_source || _target->isHidden() || _source->isHidden()
		|| !_enabled || !_target->isVisibleTo(_target->window())
		|| _target->window()->isHidden()) {
		return;
	}
	const auto before = _surface->checksum();
	if (_controller && _sampler) {
		PrepareChat(*_surface, _sampler.data(), _controller, _bounds ? _bounds() : QRect());
	} else {
		PrepareBackdrop(*_surface, _target, _source.data());
	}
	if (_surface->checksum() != before) {
		_target->update();
	}
}

} // namespace

void PrepareBackdrop(
		Surface &surface,
		not_null<QWidget*> target,
		not_null<QWidget*> source) {
	surface.setTintOpacity(st::glassTintOpacity);
	surface.setBlurRadius(st::glassBlurRadius);
	if (!BlurEnabled()) {
		surface.clear();
		return;
	}
	Expects(!source->isAncestorOf(target));
	const auto offset = MapToWidget(target, source);
	surface.prepare(target->rect(), target->devicePixelRatioF(), [&](QPainter &p, QRect clip) {
		p.fillRect(clip, st::windowBg);
		RenderRegion(p, source, offset, clip);
	}, MapToWidget(target, target->window()));
}

std::unique_ptr<QObject> ObserveChatBackdrop(
		not_null<RpWidget*> target,
		not_null<QWidget*> sampler,
		not_null<Window::SessionController*> controller,
		not_null<Surface*> surface,
		Fn<QRect()> bounds) {
	return std::make_unique<Backdrop>(target, controller->content(), surface,
		controller, sampler, std::move(bounds));
}

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

void BindButtonFeedback(not_null<AbstractButton*> button) {
	const auto state = button->lifetime().make_state<ButtonFeedback>();
	const auto reset = [=] {
		state->hover.stop();
		state->press.stop();
		button->update();
	};
	rpl::merge(
		EnabledValue() | rpl::to_empty,
		PowerSaving::Changes()
	) | rpl::on_next(reset, button->lifetime());
	button->events() | rpl::on_next([=](not_null<QEvent*> event) {
		if (event->type() == QEvent::Hide) {
			state->over = state->down = false;
			reset();
		} else if (event->type() == QEvent::FocusIn
			|| event->type() == QEvent::FocusOut) {
			button->update();
		}
	}, button->lifetime());
	button->paintRequest() | rpl::on_next([=] {
		if (!Enabled() || !button->isEnabled() || button->isDisabled()) {
			state->hover.stop();
			state->press.stop();
			state->over = state->down = false;
			return;
		}
		const auto animate = [&](Ui::Animations::Simple &animation,
				bool &previous, bool next, crl::time duration) {
			if (anim::Disabled() || PowerSaving::On(PowerSaving::kAnimations)) {
				animation.stop();
			} else if (previous != next) {
				animation.start([=] { button->update(); },
					animation.value(previous ? 1. : 0.), next ? 1. : 0.,
					duration, anim::easeOutCubic);
			}
			previous = next;
		};
		animate(state->hover, state->over, (button->isOver() || button->hasFocus()), kHoverDuration);
		animate(state->press, state->down, button->isDown(), kPressDuration);
		auto p = QPainter(button);
		const auto area = ButtonRect(button->rect());
		PaintButtonFeedback(p, area, st::windowActiveTextFg->c,
			state->hover.value(state->over ? 1. : 0.),
			state->press.value(state->down ? 1. : 0.));
	}, button->lifetime());
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

const style::SettingsButton &MenuButtonStyle(
		const style::SettingsButton &original,
		rpl::lifetime &lifetime) {
#ifdef TDESKTOP_FORK_LIQUID_GLASS
	const auto result = lifetime.make_state<style::SettingsButton>(original);
	result->textBg = st::transparent;
	return *result;
#else
	return original;
#endif
}

const style::ScrollArea &TransparentScrollStyle(
		const style::ScrollArea &original,
		rpl::lifetime &lifetime) {
#ifdef TDESKTOP_FORK_LIQUID_GLASS
	const auto result = lifetime.make_state<style::ScrollArea>(original);
	result->bg = st::transparent;
	return *result;
#else
	return original;
#endif
}

object_ptr<RpWidget> WrapChatContent(object_ptr<RpWidget> content) {
	return object_ptr<ChatContent>(content->parentWidget(), std::move(content));
}

void SetChatReadingGeometry(not_null<ElasticScroll*> scroll, QRect reading, int top, int bottom) {
	if (const auto content = dynamic_cast<ChatContent*>(scroll->widget())) {
		content->setInsets(top, bottom);
	}
	scroll->parentWidget()->setProperty("forkChatScroll", QVariant::fromValue<QObject*>(scroll));
	scroll->setGeometry(reading.adjusted(0, -top, 0, bottom));
	scroll->lower();
}

int ChatReadingHeight(not_null<ElasticScroll*> scroll) {
	const auto content = dynamic_cast<ChatContent*>(scroll->widget());
	return std::max(0, scroll->height() - (content ? content->reserve() : 0));
}

QRect ChatReadingRect(not_null<ElasticScroll*> scroll) {
	const auto content = dynamic_cast<ChatContent*>(scroll->widget());
	return content
		? scroll->geometry().adjusted(0, content->top(), 0, -content->bottom())
		: scroll->geometry();
}

void MaskCapsules(not_null<QWidget*> widget, const std::vector<QRect> &rects, int radius) {
	if (!Enabled()) {
		widget->clearMask();
		widget->setAttribute(Qt::WA_TransparentForMouseEvents, false);
		return;
	}
	auto path = QPainterPath();
	path.setFillRule(Qt::WindingFill);
	for (const auto &rect : rects) {
		if (!rect.isEmpty()) {
			const auto r = radius ? radius : rect.height() / 2.;
			path.addRoundedRect(QRectF(rect).adjusted(-st::lineWidth, -st::lineWidth, st::lineWidth, st::lineWidth), r, r);
		}
	}
	const auto mask = QRegion(path.toFillPolygon().toPolygon());
	widget->setAttribute(Qt::WA_TransparentForMouseEvents, mask.isEmpty());
	if (!mask.isEmpty()) {
		widget->setMask(mask);
	}
}

void MaskOverlayChildren(not_null<RpWidget*> container) {
	if (!Enabled()) {
		container->clearMask();
		container->setAttribute(Qt::WA_TransparentForMouseEvents, false);
		return;
	}
	auto mask = QRegion();
	for (const auto child : container->children()) {
		const auto widget = dynamic_cast<QWidget*>(child);
		if (widget && !widget->isHidden() && !widget->testAttribute(Qt::WA_TransparentForMouseEvents)) {
			mask += (widget->mask().isEmpty() ? QRegion(widget->rect()) : widget->mask())
				.translated(widget->pos());
		}
	}
	container->setAttribute(Qt::WA_TransparentForMouseEvents, mask.isEmpty());
	if (!mask.isEmpty()) {
		container->setMask(mask);
	}
}

void PrepareChat(
		Surface &surface,
		not_null<QWidget*> widget,
		not_null<Window::SessionController*> controller,
		QRect bounds) {
	surface.setTintOpacity(st::glassTintOpacity);
	surface.setBlurRadius(st::glassBlurRadius);
	if (!BlurEnabled()) {
		surface.clear();
		return;
	}
	const auto content = controller->content();
	const auto origin = MapToWidget(widget, content);
	const auto offset = QPoint(0, origin.y() - content->backgroundFromY());
	surface.prepare(bounds.isEmpty() ? widget->rect() : bounds,
		widget->devicePixelRatioF(),
		[&](QPainter &p, QRect clip) {
			p.fillRect(clip, st::windowBg);
			p.translate(-offset);
			Window::SectionWidget::PaintBackground(
				p,
				controller->currentChatTheme(),
				QSize(widget->width(), content->height()),
				clip.translated(offset),
				true);
			p.translate(offset);
			if (const auto source = ChatSource(widget)) {
				RenderRegion(p, source, MapToWidget(widget, source), clip);
			}
		}, MapToWidget(widget, widget->window()));
}

} // namespace Ui::Glass
