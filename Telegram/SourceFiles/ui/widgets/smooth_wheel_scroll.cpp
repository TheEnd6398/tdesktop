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

};

template <typename Scroll>
class WheelMotion final : public WheelMotionBase {
public:
	explicit WheelMotion(not_null<Scroll*> scroll);
	~WheelMotion() override;
	bool wheel(not_null<QWheelEvent*> event);
	void cancel() override;

private:
	bool eventFilter(QObject *object, QEvent *event) override;
	void tick(crl::time now);

	const not_null<Scroll*> _scroll;
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
WheelMotion<Scroll>::WheelMotion(not_null<Scroll*> scroll)
: WheelMotionBase(scroll)
, _scroll(scroll)
, _model(kWheelDuration) {
	setObjectName(u"liquidGlassWheel"_q);
	_clock.init([=](crl::time now) { tick(now); });
	QCoreApplication::instance()->installEventFilter(this);
	PowerSaving::Changes() | rpl::on_next([=] {
		_enabled = !PowerSaving::On(PowerSaving::kSmoothScrolling);
		cancel();
	}, _lifetime);
	_enabled = !PowerSaving::On(PowerSaving::kSmoothScrolling);
	_scroll->scrollTopChanges() | rpl::on_next([=](int) {
		if (!_writing) {
			cancel();
		}
	}, _lifetime);
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
	const auto distance = -double(event->angleDelta().y()) * (step / 120.);
	const auto position = _scroll->scrollTop();
	const auto maximum = std::max(0, _scroll->scrollTopMax());
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
		|| _scroll->scrollTopMax() != _maximum
		|| (_lastApplied >= 0 && _scroll->scrollTop() != _lastApplied)) {
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
	_scroll->scrollToY(*value);
	if (!weak) {
		return;
	}
	_writing = false;
	if (_scroll->scrollTop() != *value || _scroll->scrollTopMax() != _maximum) {
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

} // namespace

void SetupSmoothWheelScroll(not_null<ScrollArea*> scroll) {
	Install(scroll);
}

void SetupSmoothWheelScroll(not_null<ElasticScroll*> scroll) {
	Install(scroll);
}

void ResetSmoothWheelScroll(not_null<QWidget*> scroll) {
	const auto object = scroll->findChild<QObject*>(u"liquidGlassWheel"_q,
		Qt::FindDirectChildrenOnly);
	if (const auto state = dynamic_cast<WheelMotionBase*>(object)) {
		state->cancel();
	}
}

} // namespace Ui
