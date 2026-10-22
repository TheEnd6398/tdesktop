#include "ui/effects/motion_model.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

void Require(bool condition, const char *message) {
	if (!condition) {
		std::cerr << message << std::endl;
		std::exit(1);
	}
}

} // namespace

int main() {
	using namespace Ui::Motion;
	for (const auto hz : { 60., 120., 144., 240., 360. }) {
		auto model = SpringModel();
		model.retarget(1., 0, Response(Role::Press));
		auto previous = 0.;
		for (auto frame = 0; frame <= int(hz); ++frame) {
			model.advance(std::lround(1000. * frame / hz));
			Require(model.value() >= previous && model.value() <= 1.,
				"A resting spring overshot or reversed.");
			previous = model.value();
		}
		Require(!model.active() && model.value() == 1. && !model.velocity(),
			"A spring did not settle and release its clock.");
	}
	auto spring = SpringModel();
	spring.retarget(1., 100, 150);
	spring.advance(143);
	const auto position = spring.value();
	const auto velocity = spring.velocity();
	spring.retarget(0., 143, 150);
	Require(spring.value() == position && spring.velocity() == velocity,
		"Retargeting lost position or velocity continuity.");
	for (auto i = 144; i != 1000; ++i) {
		if (i % 7 == 0) spring.retarget((i / 7) % 2, i, 150);
		spring.advance(i);
		Require(std::isfinite(spring.value()) && std::isfinite(spring.velocity()),
			"Rapid interruption produced invalid physics.");
	}
	spring.reset(0.6);
	Require(!spring.active() && spring.value() == 0.6 && !spring.velocity(),
		"Cancellation did not settle immediately.");
	auto a = SpringModel(), b = SpringModel();
	a.retarget(500., 0, 220);
	b.retarget(500., 0, 220);
	for (auto i = 0; i <= 100; ++i) a.advance(i);
	b.advance(100);
	Require(std::abs(a.value() - b.value()) < 1e-12,
		"Spring results depend on frame delivery frequency.");
	Require(FrameInterval(60.) == 16 && FrameInterval(120.) == 8
		&& FrameInterval(240.) == 4 && FrameInterval(360.) == 2
		&& FrameInterval(0.) == 16 && FrameInterval(NAN) == 16,
		"Refresh pacing does not support display capabilities safely.");
	std::cout << "Motion physics, interruptions, cancellation and refresh pacing passed.\n";
}
