#pragma once

#include <QtWidgets/QWidget>

namespace Ui::Glass {

class ChatContentInsets final : public QWidget {
public:
	ChatContentInsets(QWidget *parent, QWidget *content);
	void setInsets(int top, int bottom);
	[[nodiscard]] int reserve() const { return _top + _bottom; }
	[[nodiscard]] int top() const { return _top; }
	[[nodiscard]] int bottom() const { return _bottom; }

private:
	bool eventFilter(QObject *object, QEvent *event) override;
	void refresh();

	QWidget *const _content;
	int _top = 0;
	int _bottom = 0;
};

} // namespace Ui::Glass
