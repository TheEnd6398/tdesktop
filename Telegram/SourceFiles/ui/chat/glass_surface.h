#pragma once

#include "ui/chat/glass_material.h"
#include "base/basic_types.h"
#include <rpl/producer.h>
#include <rpl/lifetime.h>
#include <memory>

class QObject;

namespace Window {
class SessionController;
} // namespace Window

namespace style {
struct InputField;
struct SettingsButton;
struct ScrollArea;
} // namespace style

namespace Ui {
class AbstractButton;
class RpWidget;
} // namespace Ui

namespace Ui::Glass {

[[nodiscard]] bool Enabled();
void SetEnabled(bool enabled);
[[nodiscard]] rpl::producer<bool> EnabledValue();
[[nodiscard]] bool BlurEnabled();
[[nodiscard]] QRect ButtonRect(QRect rect);
void BindButtonFeedback(not_null<AbstractButton*> button);
[[nodiscard]] const style::InputField &FieldStyle(
	const style::InputField &original,
	rpl::lifetime &lifetime);
[[nodiscard]] const style::SettingsButton &MenuButtonStyle(
	const style::SettingsButton &original,
	rpl::lifetime &lifetime);
[[nodiscard]] const style::ScrollArea &TransparentScrollStyle(
	const style::ScrollArea &original,
	rpl::lifetime &lifetime);

void PrepareChat(
	Surface &surface,
	not_null<QWidget*> widget,
	not_null<Window::SessionController*> controller,
	QRect bounds = {});
void PrepareBackdrop(
	Surface &surface,
	not_null<QWidget*> target,
	not_null<QWidget*> source);

[[nodiscard]] std::unique_ptr<QObject> ObserveBackdrop(
	not_null<RpWidget*> target,
	not_null<QWidget*> source,
	not_null<Surface*> surface);

} // namespace Ui::Glass
