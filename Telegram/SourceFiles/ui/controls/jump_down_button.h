/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "ui/widgets/buttons.h"
#include "ui/chat/glass_surface.h"

namespace Ui {

class JumpDownButton : public RippleButton {
public:
	JumpDownButton(QWidget *parent, const style::TwoIconButton &st);

	void setUnreadCount(int unreadCount);
	void setBackdropSource(not_null<QWidget*> source);
	[[nodiscard]] int unreadCount() const {
		return _unreadCount;
	}

protected:
	void paintEvent(QPaintEvent *e) override;

	QImage prepareRippleMask() const override;
	QPoint prepareRippleStartPosition() const override;

private:
	const style::TwoIconButton &_st;

	int _unreadCount = 0;
	Glass::Surface _glass;
	std::unique_ptr<QObject> _glassBackdrop;

};

} // namespace Ui
