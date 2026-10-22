#pragma once

#include "ui/chat/glass_material.h"
#include "base/basic_types.h"
#include "base/object_ptr.h"
#include <rpl/producer.h>
#include <rpl/lifetime.h>
#include <memory>
#include <vector>

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
class ElasticScroll;
class RpWidget;
} // namespace Ui

namespace Ui::Glass {

[[nodiscard]] bool Enabled();
void SetEnabled(bool enabled);
[[nodiscard]] rpl::producer<bool> EnabledValue();
[[nodiscard]] bool BlurEnabled();
void PaintPanel(
	QPainter &p,
	QRect rect,
	QColor tint,
	Material material = Material::Frosted,
	int radius = 0);
[[nodiscard]] QRect ButtonRect(QRect rect);
void BindButtonFeedback(not_null<AbstractButton*> button);
void SetChatBackdropPreparation(not_null<RpWidget*> owner, Fn<void()> prepare);
[[nodiscard]] const style::InputField &FieldStyle(
	const style::InputField &original,
	rpl::lifetime &lifetime);
[[nodiscard]] const style::SettingsButton &MenuButtonStyle(
	const style::SettingsButton &original,
	rpl::lifetime &lifetime);
[[nodiscard]] const style::ScrollArea &TransparentScrollStyle(
	const style::ScrollArea &original,
	rpl::lifetime &lifetime);

[[nodiscard]] object_ptr<QWidget> WrapChatContent(object_ptr<RpWidget> content);
void SetChatReadingGeometry(not_null<ElasticScroll*> scroll, QRect reading, int top, int bottom);
[[nodiscard]] int ChatReadingHeight(not_null<ElasticScroll*> scroll);
[[nodiscard]] QRect ChatReadingRect(not_null<ElasticScroll*> scroll);
void MaskCapsules(not_null<QWidget*> widget, const std::vector<QRect> &rects, int radius = 0);
void MaskOverlayChildren(not_null<RpWidget*> container);

void PrepareChat(
	Surface &surface,
	not_null<QWidget*> widget,
	not_null<Window::SessionController*> controller,
	QRect bounds = {});
void PrepareBackdrop(
	Surface &surface,
	not_null<QWidget*> target,
	not_null<QWidget*> source,
	QColor shade = {});

[[nodiscard]] std::unique_ptr<QObject> ObserveChatBackdrop(
	not_null<RpWidget*> target,
	not_null<QWidget*> sampler,
	not_null<Window::SessionController*> controller,
	not_null<Surface*> surface,
	Fn<QRect()> bounds = {});

[[nodiscard]] std::unique_ptr<QObject> ObserveBackdrop(
	not_null<RpWidget*> target,
	not_null<QWidget*> source,
	not_null<Surface*> surface,
	Fn<QColor()> shade = {});

} // namespace Ui::Glass
