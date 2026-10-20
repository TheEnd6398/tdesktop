#pragma once

#include "ui/widgets/buttons.h"

namespace Ui::Glass {

class NavigationButton final : public Ui::SettingsButton {
public:
	using Ui::SettingsButton::SettingsButton;

protected:
	void paintEvent(QPaintEvent *event) override;
	void onStateChanged(State was, StateChangeSource source) override;

private:
	Ui::Animations::Simple _hover;

};

} // namespace Ui::Glass
