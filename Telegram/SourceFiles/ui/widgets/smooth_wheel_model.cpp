#include "ui/widgets/smooth_wheel_model.h"

#include <QtGui/QWheelEvent>
#include <QtGui/QPointingDevice>
#include <algorithm>
#include <cmath>

namespace Ui {

bool IsDiscreteWheel(const QWheelEvent &event, bool horizontal) {
	const auto delta = event.angleDelta();
	const auto angle = horizontal ? QPoint(delta.y(), delta.x()) : delta;
	const auto device = event.pointingDevice();
	return event.pixelDelta().isNull()
		&& event.phase() == Qt::NoScrollPhase
		&& event.modifiers() == Qt::NoModifier
		&& angle.y() != 0
		&& std::abs(double(angle.y())) > std::abs(double(angle.x()))
		&& (!device || device->type() != QInputDevice::DeviceType::TouchPad);
}

SmoothWheelModel::SmoothWheelModel(std::int64_t duration)
: _duration(std::max(std::int64_t(1), duration)) {
}

bool SmoothWheelModel::scrollBy(
		double distance,
		int position,
		int maximum,
		std::int64_t now) {
	if (!std::isfinite(distance) || !distance || maximum < 0
		|| position < 0 || position > maximum) {
		cancel();
		return false;
	}
	const auto direction = (distance > 0.) ? 1 : -1;
	if (_active) {
		if (!advance(now)) {
			_velocity = 0.;
		}
	}
	if (_direction && direction != _direction) {
		_active = false;
		_velocity = 0.;
		_remainder = 0.;
	}
	_direction = direction;
	const auto accumulated = _remainder + distance;
	const auto whole = std::trunc(accumulated);
	_remainder = accumulated - whole;
	if (!whole) {
		return true;
	}
	const auto base = _active ? _target : position;
	_target = int(std::clamp(base + whole, 0., double(maximum)));
	_from = position;
	_slope = _active && _target != _from
		? std::clamp(_velocity * _duration / (_target - _from), 0., 3.)
		: 3.;
	_started = now;
	_active = (_target != _from);
	return true;
}

std::optional<int> SmoothWheelModel::advance(std::int64_t now) {
	if (!_active) {
		return std::nullopt;
	}
	const auto progress = std::clamp((now - _started) / double(_duration), 0., 1.);
	const auto square = progress * progress;
	const auto cube = square * progress;
	const auto eased = 3. * square - 2. * cube
		+ _slope * (cube - 2. * square + progress);
	_velocity = (_target - _from) / double(_duration)
		* (6. * progress * (1. - progress)
			+ _slope * (3. * square - 4. * progress + 1.));
	const auto value = int(std::lround(_from + (_target - _from) * eased));
	if (progress >= 1.) {
		_active = false;
	}
	return value;
}

bool SmoothWheelModel::active() const {
	return _active;
}

int SmoothWheelModel::target() const {
	return _target;
}

void SmoothWheelModel::cancel() {
	_active = false;
	_velocity = 0.;
	_remainder = 0.;
	_direction = 0;
}

} // namespace Ui
