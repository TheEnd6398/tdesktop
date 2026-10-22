#pragma once

#include <cstdint>
#include <optional>

class QWheelEvent;

namespace Ui {

[[nodiscard]] bool IsDiscreteWheel(const QWheelEvent &event, bool horizontal = false);

class SmoothWheelModel final {
public:
	explicit SmoothWheelModel(std::int64_t duration);

	bool scrollBy(double distance, int position, int maximum, std::int64_t now);
	[[nodiscard]] std::optional<int> advance(std::int64_t now);
	[[nodiscard]] bool active() const;
	[[nodiscard]] int target() const;
	void cancel();

private:
	const std::int64_t _duration;
	std::int64_t _started = 0;
	int _from = 0;
	int _target = 0;
	int _direction = 0;
	double _slope = 3.;
	double _velocity = 0.;
	double _remainder = 0.;
	bool _active = false;

};

} // namespace Ui
