#include "ui/chat/chat_content_insets.h"

#include <QtCore/QPointer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>
#include <cstdlib>
#include <iostream>

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
	for (const auto reserves : { QSize(80, 96), QSize(144, 240), QSize(0, 0) }) {
		auto scroll = QScrollArea();
		scroll.setFrameShape(QFrame::NoFrame);
		scroll.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		scroll.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		scroll.resize(480, 320);
		const auto content = new QWidget();
		content->resize(480, 2000);
		const auto wrapper = new Ui::Glass::ChatContentInsets(nullptr, content);
		scroll.setWidget(wrapper);
		scroll.show();
		app.processEvents();
		const auto bar = scroll.verticalScrollBar();
		const auto maximum = bar->maximum();
		bar->setValue(714);
		const auto anchor = content->mapTo(scroll.viewport(), QPoint(20, 900));
		wrapper->setInsets(reserves.width(), reserves.height());
		scroll.resize(480, 320 + wrapper->reserve());
		app.processEvents();
		Check(bar->maximum() == maximum, "Overlay reserves changed the scroll range.");
		Check(bar->value() == 714, "Adding overlays moved the saved scroll position.");
		Check(content->mapTo(scroll.viewport(), QPoint(20, 900)) - QPoint(0, wrapper->top()) == anchor,
			"A message anchor moved within the reading area.");
		bar->setValue(maximum);
		Check(content->mapTo(scroll.viewport(), QPoint(0, content->height())).y()
			== scroll.viewport()->height() - wrapper->bottom(),
			"The last message is hidden behind the composer at rest.");
		wrapper->setInsets(reserves.width(), reserves.height() + 120);
		bar->setValue(bar->maximum());
		Check(content->mapTo(scroll.viewport(), QPoint(0, content->height())).y()
			== scroll.viewport()->height() - wrapper->bottom(),
			"Growing the composer broke bottom anchoring.");
		wrapper->setInsets(0, 0);
		scroll.resize(480, 320);
		bar->setValue(714);
		Check(content->mapTo(scroll.viewport(), QPoint(20, 900)) == anchor,
			"Disabling capsule layout changed the programmatic jump coordinates.");
		content->resize(480, 320);
		app.processEvents();
		Check(bar->maximum() == 0, "A short history gained an extra scroll range.");
		const auto guardContent = QPointer<QWidget>(content);
		const auto guardWrapper = QPointer<QWidget>(wrapper);
		delete scroll.takeWidget();
		Check(!guardContent && !guardWrapper, "Replacing a chat did not destroy its complete wrapped content.");
	}
	std::cout << "Chat insets: anchors, range, composer growth, toggles and short histories passed.\n";
}
