#include "ui/widgets/smooth_wheel_scroll.h"

#include "ui/widgets/smooth_wheel_model.h"
#include "ui/widgets/scroll_area.h"
#include "ui/widgets/elastic_scroll.h"
#include "ui/effects/animations.h"
#include "ui/power_saving.h"
#include "styles/style_fork_glass.h"

#include <QtCore/QPointer>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QScrollBar>

namespace Ui {
namespace {

constexpr auto kWheelDuration = crl::time(220);

class WheelMotionBase : public QObject {
public:
	using QObject::QObject;
	virtual void cancel() = 0;
	virtual bool wheel(not_null<QWheelEvent*> event) = 0;

};

template <typename Scroll>
class WheelMotion final : public WheelMotionBase {
public:
	explicit WheelMotion(not_null<Scroll*> scroll, bool horizontal = false);
	~WheelMotion() override;
	bool wheel(not_null<QWheelEvent*> event) override;
	void cancel() override;

private:
	bool eventFilter(QObject *object, QEvent *event) override;
	void tick(crl::time now);

	const not_null<Scroll*> _scroll;
	const bool _horizontal;
	SmoothWheelModel _model;
	Ui::Animations::Basic _clock;
	int _lastApplied = -1;
	int _maximum = 0;
	bool _writing = false;
	bool _enabled = false;
	rpl::lifetime _lifetime;

};

int WheelStep(not_null<Ui::ScrollArea*> scroll) {
	return scroll->verticalScrollBar()->singleStep();
}

int WheelStep(not_null<Ui::ElasticScroll*>) {
	return st::glassWheelStep;
}

template <typename Scroll>
int Position(not_null<Scroll*> scroll, bool horizontal) {
	return horizontal ? scroll->scrollLeft() : scroll->scrollTop();
}

template <typename Scroll>
int Maximum(not_null<Scroll*> scroll, bool horizontal) {
	return horizontal ? scroll->scrollLeftMax() : scroll->scrollTopMax();
}

template <typename Scroll>
void Apply(not_null<Scroll*> scroll, bool horizontal, int value) {
	if constexpr (std::is_same_v<Scroll, ScrollArea>) {
		if (horizontal) {
			scroll->scrollToX(value);
			return;
		}
	}
	scroll->scrollToY(value);
}

template <typename Scroll>
WheelMotion<Scroll>::WheelMotion(not_null<Scroll*> scroll, bool horizontal)
: WheelMotionBase(scroll)
, _scroll(scroll)
, _horizontal(horizontal)
, _model(kWheelDuration) {
	setObjectName(horizontal ? u"liquidGlassHorizontalWheel"_q : u"liquidGlassWheel"_q);
	_clock.init([=](crl::time now) { tick(now); });
	QCoreApplication::instance()->installEventFilter(this);
	PowerSaving::Changes() | rpl::on_next([=] {
		_enabled = (!PowerSaving::On(PowerSaving::kSmoothScrolling)
		&& !PowerSaving::On(PowerSaving::kAnimations));
		cancel();
	}, _lifetime);
	_enabled = (!PowerSaving::On(PowerSaving::kSmoothScrolling)
		&& !PowerSaving::On(PowerSaving::kAnimations));
	if (_horizontal) {
		_scroll->scrolls() | rpl::on_next([=] {
			if (!_writing) cancel();
		}, _lifetime);
	} else {
	_scroll->scrollTopChanges() | rpl::on_next([=](int) {
		if (!_writing) {
			cancel();
		}
	}, _lifetime);
	}
	rpl::merge(_scroll->innerResizes(), _scroll->geometryChanged())
	| rpl::on_next([=] { cancel(); }, _lifetime);
}

template <typename Scroll>
WheelMotion<Scroll>::~WheelMotion() {
	if (const auto app = QCoreApplication::instance()) {
		app->removeEventFilter(this);
	}
	cancel();
}

template <typename Scroll>
void WheelMotion<Scroll>::cancel() {
	_clock.stop();
	_model.cancel();
	_lastApplied = -1;
}

template <typename Scroll>
bool WheelMotion<Scroll>::wheel(not_null<QWheelEvent*> event) {
	if (!_enabled || anim::Disabled() || !IsDiscreteWheel(*event)
		|| _scroll->isHidden() || !_scroll->isVisibleTo(_scroll->window())
		|| _scroll->window()->isHidden()) {
		cancel();
		return false;
	}
	const auto lines = QApplication::wheelScrollLines();
	const auto step = (lines < 0)
		? double(_scroll->height())
		: double(lines) * WheelStep(_scroll);
	const auto distance = -double(event->angleDelta().y()) * (_horizontal ? 1. : step / 120.);
	const auto position = Position(_scroll, _horizontal);
	const auto maximum = std::max(0, Maximum(_scroll, _horizontal));
	if (!step || position < 0 || position > maximum
		|| position + distance <= 0 || position + distance >= maximum) {
		cancel();
		return false;
	}
	_maximum = maximum;
	_lastApplied = position;
	const auto consumed = _model.scrollBy(distance, position, maximum, crl::now());
	if (_model.active()) {
		if (!_clock.animating()) {
			_clock.start();
		}
	} else {
		_clock.stop();
	}
	return consumed;
}

template <typename Scroll>
void WheelMotion<Scroll>::tick(crl::time now) {
	if (!_enabled || anim::Disabled() || _scroll->isHidden()
		|| !_scroll->isVisibleTo(_scroll->window())
		|| _scroll->window()->isHidden()
		|| Maximum(_scroll, _horizontal) != _maximum
		|| (_lastApplied >= 0 && Position(_scroll, _horizontal) != _lastApplied)) {
		cancel();
		return;
	}
	const auto value = _model.advance(now);
	if (!value) {
		_clock.stop();
		return;
	}
	const auto weak = QPointer<WheelMotionBase>(this);
	_writing = true;
	Apply(_scroll, _horizontal, *value);
	if (!weak) {
		return;
	}
	_writing = false;
	if (Position(_scroll, _horizontal) != *value || Maximum(_scroll, _horizontal) != _maximum) {
		cancel();
		return;
	}
	_lastApplied = *value;
	if (!_model.active()) {
		_clock.stop();
	}
}

template <typename Scroll>
bool WheelMotion<Scroll>::eventFilter(QObject *object, QEvent *event) {
	if (!_model.active() || !object->isWidgetType()) {
		return false;
	}
	const auto widget = static_cast<QWidget*>(object);
	switch (event->type()) {
	case QEvent::KeyPress:
	case QEvent::MouseButtonPress:
	case QEvent::TouchBegin:
		if (widget->window() == _scroll->window()) {
			cancel();
		}
		break;
	case QEvent::Hide:
	case QEvent::Resize:
	case QEvent::ChildAdded:
	case QEvent::WindowDeactivate:
		if (widget == _scroll || widget->isAncestorOf(_scroll)) {
			cancel();
		}
		break;
	default:
		break;
	}
	return false;
}

template <typename Scroll>
void Install(not_null<Scroll*> scroll) {
#ifdef TDESKTOP_FORK_LIQUID_GLASS
	if (scroll->template findChild<QObject*>(u"liquidGlassWheel"_q,
			Qt::FindDirectChildrenOnly)) {
		return;
	}
	const auto state = new WheelMotion<Scroll>(scroll);
	scroll->setCustomWheelProcess([=](not_null<QWheelEvent*> event) {
		return state->wheel(event);
	});
#endif
}

template <typename Scroll>
bool Process(not_null<Scroll*> scroll, not_null<QWheelEvent*> event, bool horizontal) {
	const auto name = horizontal ? u"liquidGlassHorizontalWheel"_q : u"liquidGlassWheel"_q;
	auto state = dynamic_cast<WheelMotionBase*>(scroll->template findChild<QObject*>(
		name, Qt::FindDirectChildrenOnly));
	if (!IsDiscreteWheel(*event) || !Maximum(scroll, horizontal)) {
		if (state) state->cancel();
		return false;
	}
	if (!state) state = new WheelMotion<Scroll>(scroll, horizontal);
	return state->wheel(event);
}

} // namespace

void SetupSmoothWheelScroll(not_null<ScrollArea*> scroll) {
	Install(scroll);
}

void SetupSmoothWheelScroll(not_null<ElasticScroll*> scroll) {
	Install(scroll);
}

bool ProcessSmoothWheelScroll(
		not_null<QWidget*> scroll,
		not_null<QWheelEvent*> event,
		bool horizontal) {
#ifdef TDESKTOP_FORK_LIQUID_GLASS
	if (const auto area = dynamic_cast<ScrollArea*>(scroll.get())) {
		return Process(not_null<ScrollArea*>(area), event, horizontal);
	} else if (!horizontal) {
		if (const auto elastic = dynamic_cast<ElasticScroll*>(scroll.get())) {
			return Process(not_null<ElasticScroll*>(elastic), event, false);
		}
	}
#endif
	return false;
}

void ResetSmoothWheelScroll(not_null<QWidget*> scroll) {
	const auto object = scroll->findChild<QObject*>(u"liquidGlassWheel"_q,
		Qt::FindDirectChildrenOnly);
	if (const auto state = dynamic_cast<WheelMotionBase*>(object)) {
		state->cancel();
	}
}

} // namespace Ui
