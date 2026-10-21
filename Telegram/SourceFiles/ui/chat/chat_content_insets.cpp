#include "ui/chat/chat_content_insets.h"

#include <QtCore/QEvent>
#include <algorithm>

namespace Ui::Glass {

ChatContentInsets::ChatContentInsets(QWidget *parent, QWidget *content)
: QWidget(parent)
, _content(content) {
	_content->setParent(this);
	_content->installEventFilter(this);
	_content->show();
	refresh();
}

void ChatContentInsets::setInsets(int top, int bottom) {
	_top = std::max(0, top);
	_bottom = std::max(0, bottom);
	refresh();
}

void ChatContentInsets::refresh() {
	_content->move(0, _top);
	resize(_content->width(), _content->height() + reserve());
}

bool ChatContentInsets::eventFilter(QObject *object, QEvent *event) {
	if (object == _content && event->type() == QEvent::Resize) {
		refresh();
	}
	return false;
}

} // namespace Ui::Glass
