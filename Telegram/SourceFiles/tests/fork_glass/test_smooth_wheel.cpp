#include "ui/widgets/smooth_wheel_model.h"

#include <QtGui/QPointingDevice>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QApplication>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {

void Check(bool condition, const char *message) {
	if (!condition) {
		std::cerr << message << '\n';
		std::exit(1);
	}
}

} // namespace

int main(int argc, char **argv) {
	qputenv("QT_QPA_PLATFORM", "offscreen");
	auto app = QApplication(argc, argv);
	auto mouse = QPointingDevice(QString::fromUtf16(u"mouse"), 1, QInputDevice::DeviceType::Mouse,
		QPointingDevice::PointerType::Generic, QInputDevice::Capability::Position, 1, 3);
	auto touchpad = QPointingDevice(QString::fromUtf16(u"touchpad"), 2, QInputDevice::DeviceType::TouchPad,
		QPointingDevice::PointerType::Finger, QInputDevice::Capability::Position, 1, 0);
	const auto accepts = [&](QPoint pixels, QPoint angle, Qt::ScrollPhase phase,
			Qt::KeyboardModifiers modifiers, const QPointingDevice *device) {
		const auto event = QWheelEvent(QPointF(), QPointF(), pixels, angle,
			Qt::NoButton, modifiers, phase, false, Qt::MouseEventNotSynthesized, device);
		return Ui::IsDiscreteWheel(event);
	};
	Check(accepts({}, { 0, 120 }, Qt::NoScrollPhase, {}, &mouse), "Mouse detent was rejected.");
	Check(accepts({}, { 0, 15 }, Qt::NoScrollPhase, {}, &mouse), "Fractional wheel angle was rejected.");
	Check(!accepts({ 0, 1 }, { 0, 120 }, Qt::NoScrollPhase, {}, &mouse), "Pixel scrolling was intercepted.");
	Check(!accepts({}, { 120, 0 }, Qt::NoScrollPhase, {}, &mouse), "Horizontal scrolling was intercepted.");
	Check(!accepts({}, { 0, 120 }, Qt::NoScrollPhase, {}, &touchpad), "Precision touchpad was intercepted.");
	for (const auto phase : { Qt::ScrollBegin, Qt::ScrollUpdate, Qt::ScrollMomentum, Qt::ScrollEnd }) {
		Check(!accepts({}, { 0, 120 }, phase, {}, &mouse), "A phased gesture was intercepted.");
	}
	for (const auto modifier : { Qt::ControlModifier, Qt::ShiftModifier, Qt::AltModifier, Qt::MetaModifier }) {
		Check(!accepts({}, { 0, 120 }, Qt::NoScrollPhase, modifier, &mouse), "Modified wheel input was intercepted.");
	}

	const auto horizontal = QWheelEvent(QPointF(), QPointF(), {}, { 120, 0 },
		Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false,
		Qt::MouseEventNotSynthesized, &mouse);
	Check(Ui::IsDiscreteWheel(horizontal, true), "Native horizontal mouse detents were rejected.");
	Check(!Ui::IsDiscreteWheel(horizontal), "Horizontal input entered the vertical easing path.");
	const auto horizontalPad = QWheelEvent(QPointF(), QPointF(), { 8, 0 }, { 120, 0 },
		Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false,
		Qt::MouseEventNotSynthesized, &touchpad);
	Check(!Ui::IsDiscreteWheel(horizontalPad, true), "Horizontal touchpad input was double animated.");

	auto motion = Ui::SmoothWheelModel(220);
	Check(motion.scrollBy(60., 100, 1000, 0), "Wheel input was not consumed.");
	auto position = 100;
	for (auto now = 0; now <= 220; now += 10) {
		const auto value = motion.advance(now);
		Check(value && *value >= position && *value <= 160, "Scroll overshot or reversed.");
		position = *value;
	}
	Check(position == 160 && !motion.active(), "Scroll did not finish at its target.");
	Check(!motion.advance(1000), "An idle scroll still produces frames.");

	position = 100;
	for (auto i = 0; i != 100; ++i) {
		motion.scrollBy(0.25, position, 1000, i * 300);
		if (const auto value = motion.advance(i * 300 + 220)) {
			position = *value;
		}
	}
	Check(position == 125, "Fractional wheel distances were lost.");

	motion.cancel();
	motion.scrollBy(60., 100, 1000, 0);
	position = *motion.advance(40);
	motion.scrollBy(60., position, 1000, 40);
	Check(motion.target() == 220, "Repeated wheel input did not accumulate.");
	auto previous = position;
	for (auto now = 41; now < 80; ++now) {
		const auto next = motion.advance(now);
		Check(next && *next >= previous && *next <= 220,
			"Interrupted wheel interpolation overshot or reversed.");
		previous = *next;
	}
	position = *motion.advance(80);
	motion.scrollBy(-20., position, 1000, 80);
	Check(motion.target() == position - 20, "Direction reversal retained the old target.");
	Check(motion.advance(1000) == position - 20, "Reversal did not land on the new target.");

	motion.scrollBy(1000000., 100, 500, 0);
	Check(motion.advance(1000) == 500, "Bottom range was exceeded.");
	motion.scrollBy(-1000000., 500, 500, 2000);
	Check(motion.advance(3000) == 0, "Top range was exceeded.");
	motion.scrollBy(60., 100, 1000, 0);
	motion.cancel();
	Check(!motion.advance(1000), "Cancellation still changes the viewport.");
	motion.scrollBy(60., 700, 1000, 2000);
	Check(motion.target() == 760, "Programmatic position was not respected.");
	Check(!motion.scrollBy(std::numeric_limits<double>::infinity(), 0, 1000, 0), "Non-finite input was accepted.");
	Check(!motion.active(), "Invalid input left an animation running.");
	std::cout << "Smooth wheel: input classification, easing, fractions, accumulation, reversal, bounds and cancellation passed.\n";
}
