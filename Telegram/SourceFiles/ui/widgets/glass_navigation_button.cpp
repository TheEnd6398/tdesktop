#include "ui/widgets/glass_navigation_button.h"

#include "ui/chat/glass_surface.h"
#include "ui/power_saving.h"
#include "ui/painter.h"
#include "styles/style_fork_glass.h"

#include <QtGui/QPainterPath>

namespace Ui::Glass {
namespace {

constexpr auto kHoverDuration = crl::time(120);

} // namespace

void NavigationButton::paintEvent(QPaintEvent *event) {
	if (!Enabled()) {
		Ui::SettingsButton::paintEvent(event);
		return;
	}
	if (anim::Disabled() || PowerSaving::On(PowerSaving::kAnimations)) {
		_hover.stop();
	}
	auto p = Painter(this);
	const auto over = (isOver() || isDown()) && !isDisabled();
	const auto area = rect().marginsRemoved(QMargins(
		st::glassGap, st::lineWidth, st::glassGap, st::lineWidth));
	auto path = QPainterPath();
	path.addRoundedRect(area, st::glassRowRadius, st::glassRowRadius);
	p.setRenderHint(QPainter::Antialiasing);
	p.setClipPath(path);
	const auto tint = anim::color(st().textBg, st().textBgOver,
		_hover.value(over ? 1. : 0.));
	PaintPanel(p, area, tint, Material::Soft, st::glassRowRadius);
	paintMotion(p, area, st::glassRowRadius);
	paintRipple(p, 0, 0);
	paintText(p, over, width());
	paintToggle(p, width());
}

void NavigationButton::onStateChanged(State was, StateChangeSource source) {
	Ui::SettingsButton::onStateChanged(was, source);
	if (!Enabled() || anim::Disabled()
		|| PowerSaving::On(PowerSaving::kAnimations)) {
		_hover.stop();
		return;
	}
	const auto over = (isOver() || isDown()) && !isDisabled();
	const auto wasOver = (was & (StateFlag::Over | StateFlag::Down))
		&& !(was & StateFlag::Disabled);
	if (over == wasOver) {
		return;
	}
	_hover.start([=] { update(); }, _hover.value(over ? 0. : 1.),
		over ? 1. : 0., Motion::Duration(Motion::Role::Hover, kHoverDuration),
		Motion::Curve(Motion::Role::Hover));
}

} // namespace Ui::Glass
