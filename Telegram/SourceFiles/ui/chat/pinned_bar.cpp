/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "ui/chat/pinned_bar.h"

#include "ui/chat/message_bar.h"
#include "ui/power_saving.h"
#include "ui/effects/spoiler_mess.h"
#include "ui/widgets/shadow.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/fade_wrap.h"
#include "styles/style_chat_helpers.h"
#include "styles/style_fork_glass.h"
#include "styles/palette.h"

#include <QtGui/QtEvents>

namespace Ui {

PinnedBar::PinnedBar(
	not_null<QWidget*> parent,
	Fn<bool()> customEmojiPaused,
	rpl::producer<> customEmojiPausedChanges,
	Window::SessionController *controller)
: _wrap(parent, object_ptr<RpWidget>(parent))
, _controller(controller)
, _shadow(std::make_unique<PlainShadow>(_wrap.parentWidget()))
, _customEmojiPaused(std::move(customEmojiPaused)) {
	_wrap.hide(anim::type::instant);
	_shadow->hide();

	_shadow->showOn(rpl::combine(
		_wrap.shownValue(),
		_wrap.heightValue(),
		rpl::mappers::_1 && rpl::mappers::_2 > 0
	) | rpl::filter([=](bool shown) {
		return (shown == _shadow->isHidden());
	}));

	_wrap.entity()->paintRequest(
	) | rpl::on_next([=](QRect clip) {
		auto p = QPainter(_wrap.entity());
		if (Glass::Enabled()) {
			if (_controller) {
				Glass::PrepareChat(_glass, _wrap.entity(), _controller);
			}
			_glass.paint(p, _wrap.entity()->rect().marginsRemoved(QMargins(
				st::glassMargin, st::glassGap, st::glassMargin, st::glassGap)),
				st::historyPinnedBg->c);
		} else {
			p.fillRect(clip, st::historyPinnedBg);
		}
	}, lifetime());
	_wrap.setAttribute(Qt::WA_OpaquePaintEvent, !Glass::Enabled());
	Glass::EnabledValue() | rpl::on_next([=] {
		_wrap.setAttribute(Qt::WA_OpaquePaintEvent, !Glass::Enabled());
		if (_bar) {
			_wrap.entity()->resize(_wrap.entity()->width(),
				st::historyReplyHeight + (Glass::Enabled() ? 2 * st::glassGap : 0));
			updateControlsGeometry(_wrap.geometry());
		}
		_wrap.entity()->update();
	}, lifetime());
	PowerSaving::Changes() | rpl::on_next([=] {
		_wrap.entity()->update();
	}, lifetime());

	if (customEmojiPausedChanges) {
		std::move(
			customEmojiPausedChanges
		) | rpl::on_next([=] {
			_wrap.entity()->update();
		}, lifetime());
	}
}

PinnedBar::~PinnedBar() {
	_right.button.destroy();
}

void PinnedBar::setContent(rpl::producer<Ui::MessageBarContent> content) {
	_contentLifetime.destroy();

	auto copy = std::move(
		content
	) | rpl::start_spawning(_contentLifetime);

	rpl::duplicate(
		copy
	) | rpl::filter([=](const MessageBarContent &content) {
		return !content.title.isEmpty() || !content.text.text.isEmpty();
	}) | rpl::on_next([=](MessageBarContent &&content) {
		const auto creating = !_bar;
		if (creating) {
			createControls();
		}

		// In most cases the new right button should arrive
		// before we want to get its width.
		const auto right = _right.button ? _right.button->width() : 0;
		content.margins = { 0, 0, right, 0 };

		_bar->set(std::move(content));
		if (creating) {
			_bar->finishAnimating();
		}
	}, _contentLifetime);

	std::move(
		copy
	) | rpl::map([=](const MessageBarContent &content) {
		return content.title.isEmpty() || content.text.text.isEmpty();
	}) | rpl::on_next_done([=](bool hidden) {
		_shouldBeShown = !hidden;
		if (!_forceHidden) {
			_wrap.toggle(_shouldBeShown, anim::type::normal);
		} else if (!_shouldBeShown) {
			_bar = nullptr;
		}
	}, [=] {
		_forceHidden = true;
		_wrap.toggle(false, anim::type::normal);
	}, _contentLifetime);
}

void PinnedBar::setRightButton(object_ptr<Ui::RpWidget> button) {
	const auto hasPrevious = (_right.button != nullptr);
	if (auto previous = _right.button.release()) {
		using Unique = base::unique_qptr<Ui::FadeWrapScaled<Ui::RpWidget>>;
		_right.previousButtonLifetime = previous->shownValue(
		) | rpl::filter(!rpl::mappers::_1) | rpl::on_next([=] {
			_right.previousButtonLifetime.destroy();
		});
		previous->hide(anim::type::normal);
		_right.previousButtonLifetime.make_state<Unique>(Unique{ previous });
	}
	_right.button.create(_wrap.entity(), std::move(button));
	if (_right.button) {
		_right.button->setParent(_wrap.entity());
		if (hasPrevious) {
			_right.button->setDuration(st::defaultMessageBar.duration);
			_right.button->show(anim::type::normal);
		} else {
			_right.button->setDuration(0);
			_right.button->show(anim::type::instant);
		}

		// The button is created and replaced deeper in the tree than the bars
		// container watches, so it wouldn't be placed in the visual Tab order.
		RefreshVisualTabOrder(_right.button.data());
	}
	if (_bar) {
		updateControlsGeometry(_wrap.geometry());
	}
}

void PinnedBar::updateControlsGeometry(QRect wrapGeometry) {
	const auto margin = Glass::Enabled() ? st::glassMargin : 0;
	const auto gap = Glass::Enabled() ? st::glassGap : 0;
	Glass::MaskCapsules(_wrap.entity(), { _wrap.entity()->rect().marginsRemoved(
		QMargins(margin, gap, margin, gap)) });
	Glass::MaskOverlayChildren(&_wrap);
	_bar->widget()->resizeToWidth(wrapGeometry.width() - 2 * margin);
	_bar->widget()->moveToLeft(margin, gap);
	if (_right.button) {
		_right.button->moveToRight(margin, gap);
	}
}

void PinnedBar::setShadowGeometryPostprocess(Fn<QRect(QRect)> postprocess) {
	_shadowGeometryPostprocess = std::move(postprocess);
	updateShadowGeometry(_wrap.geometry());
}

void PinnedBar::updateShadowGeometry(QRect wrapGeometry) {
	const auto regular = QRect(
		wrapGeometry.x(),
		wrapGeometry.y() + wrapGeometry.height(),
		wrapGeometry.width(),
		Glass::Enabled() ? 0 : st::lineWidth);
	_shadow->setGeometry(_shadowGeometryPostprocess
		? _shadowGeometryPostprocess(regular)
		: regular);
}

void PinnedBar::createControls() {
	Expects(!_bar);

	_bar = std::make_unique<MessageBar>(
		_wrap.entity(),
		st::defaultMessageBar,
		_customEmojiPaused);
	if (_right.button) {
		_right.button->raise();
	}

	// Clicks.
	_bar->widget()->setCursor(style::cur_pointer);
	const auto fillClicks = [=](
			Qt::MouseButton button,
			rpl::event_stream<> &stream) {
		_bar->widget()->events(
		) | rpl::filter([=](not_null<QEvent*> event) {
			return (event->type() == QEvent::MouseButtonPress)
				&& (static_cast<QMouseEvent*>(event.get())->button()
						== button);
		}) | rpl::map([=] {
			return _bar->widget()->events(
			) | rpl::filter([](not_null<QEvent*> event) {
				return (event->type() == QEvent::MouseButtonRelease);
			}) | rpl::take(1) | rpl::filter([=](not_null<QEvent*> event) {
				return _bar->widget()->rect().contains(
					static_cast<QMouseEvent*>(event.get())->pos());
			});
		}) | rpl::flatten_latest(
		) | rpl::to_empty | rpl::start_to_stream(
			stream,
			_bar->widget()->lifetime());
	};
	fillClicks(Qt::LeftButton, _barClicks);
	fillClicks(Qt::RightButton, _barRightClicks);

	_bar->widget()->move(0, 0);
	_bar->widget()->show();
	_wrap.entity()->resize(
		_wrap.entity()->width(),
		_bar->widget()->height() + (Glass::Enabled() ? 2 * st::glassGap : 0));

	_wrap.geometryValue(
	) | rpl::on_next([=](QRect rect) {
		updateShadowGeometry(rect);
		updateControlsGeometry(rect);
	}, _bar->widget()->lifetime());

	_wrap.shownValue(
	) | rpl::skip(
		1
	) | rpl::filter([=](bool shown) {
		return !shown && !_forceHidden;
	}) | rpl::on_next([=] {
		_bar = nullptr;
	}, _bar->widget()->lifetime());

	Ensures(_bar != nullptr);
}

void PinnedBar::show() {
	if (!_forceHidden) {
		return;
	}
	_forceHidden = false;
	if (_shouldBeShown) {
		_wrap.show(anim::type::instant);
	} else if (!_wrap.isHidden() && !_wrap.animating()) {
		_wrap.hide(anim::type::instant);
	}
}

void PinnedBar::hide() {
	if (_forceHidden) {
		return;
	}
	_forceHidden = true;
	_wrap.hide(anim::type::instant);
}

void PinnedBar::raise() {
	_wrap.raise();
	_shadow->raise();
}

void PinnedBar::customEmojiRepaint() {
	if (_bar) {
		_bar->customEmojiRepaint();
	}
}

void PinnedBar::finishAnimating() {
	_wrap.finishAnimating();
}

void PinnedBar::move(int x, int y) {
	_wrap.move(x, y);
}

void PinnedBar::resizeToWidth(int width) {
	_wrap.entity()->resizeToWidth(width);
	if (!_wrap.width()) {
		_wrap.resizeToWidth(width);
	}
}

int PinnedBar::height() const {
	return !_forceHidden
		? _wrap.height()
		: _shouldBeShown
		? st::historyReplyHeight + (Glass::Enabled() ? 2 * st::glassGap : 0)
		: 0;
}

rpl::producer<int> PinnedBar::heightValue() const {
	return _wrap.heightValue();
}

rpl::producer<> PinnedBar::barClicks() const {
	return _barClicks.events();
}

rpl::producer<> PinnedBar::barRightClicks() const {
	return _barRightClicks.events();
}

rpl::producer<> PinnedBar::contextMenuRequested() const {
	return _wrap.entity()->paintRequest(
	) | rpl::filter([=] {
		return _bar && _bar->widget();
	}) | rpl::map([=] {
		return _bar->widget()->events(
		) | rpl::filter([](not_null<QEvent*> event) {
			return (event->type() == QEvent::ContextMenu);
		}) | rpl::to_empty;
	}) | rpl::flatten_latest();
}

} // namespace Ui
