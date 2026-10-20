#pragma once

#include <QtGui/QImage>
#include <QtGui/QPainter>

namespace Window {
class SessionController;
} // namespace Window

namespace style {
struct InputField;
} // namespace style

namespace Ui::Glass {

[[nodiscard]] bool Enabled();
void SetEnabled(bool enabled);
[[nodiscard]] rpl::producer<bool> EnabledValue();
[[nodiscard]] bool BlurEnabled();
[[nodiscard]] QRect ButtonRect(QRect rect);
[[nodiscard]] const style::InputField &FieldStyle(
	const style::InputField &original,
	rpl::lifetime &lifetime);

class Surface final {
public:
	using Source = Fn<void(QPainter &p, QRect clip)>;

	void prepare(QRect bounds, qreal dpr, const Source &source);
	void paint(QPainter &p, QRect rect, QColor tint, int radius = 0) const;
	void clear();

private:
	QImage _image;
	QImage _scratch;
	QRect _bounds;

};

void PrepareChat(
	Surface &surface,
	not_null<QWidget*> widget,
	not_null<Window::SessionController*> controller,
	QRect bounds = {});

} // namespace Ui::Glass
