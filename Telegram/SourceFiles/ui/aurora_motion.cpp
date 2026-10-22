#include "ui/aurora_motion.h"

#include "core/application.h"
#include "core/core_settings.h"
#include "ui/chat/glass_surface.h"
#include "ui/power_saving.h"
#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QCoreApplication>
#include <QtCore/QPointer>
#include <QtGui/QGuiApplication>

#ifdef Q_OS_WIN
#include <windows.h>
#endif // Q_OS_WIN

namespace Ui::AuroraMotion {
namespace {

class SystemPreferences final : public QObject, public QAbstractNativeEventFilter {
public:
	SystemPreferences() : QObject(QCoreApplication::instance()) {
		QCoreApplication::instance()->installNativeEventFilter(this);
		QObject::connect(qGuiApp, &QGuiApplication::applicationStateChanged,
			this, [=] { refresh(); });
		refresh();
	}
	~SystemPreferences() {
		if (const auto app = QCoreApplication::instance()) {
			app->removeNativeEventFilter(this);
		}
	}
	[[nodiscard]] bool reduced() const { return _reduced; }

private:
	bool nativeEventFilter(const QByteArray &, void *message, qintptr *) override {
#ifdef Q_OS_WIN
		const auto event = static_cast<MSG*>(message);
		if (event->message == WM_SETTINGCHANGE) {
			refresh();
		}
#endif // Q_OS_WIN
		return false;
	}
	void refresh() {
#ifdef Q_OS_WIN
		auto enabled = BOOL(TRUE);
		_reduced = SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION,
			0, &enabled, 0) && !enabled;
#endif // Q_OS_WIN
	}
	bool _reduced = false;
};

bool SystemReduced() {
	static auto preferences = QPointer<SystemPreferences>();
	if (!preferences) {
		preferences = new SystemPreferences();
	}
	return preferences->reduced();
}

} // namespace

Motion::Mode Mode() {
	return !Glass::Enabled()
		? Motion::Mode::Legacy
		: PowerSaving::On(PowerSaving::kAnimations)
		? Motion::Mode::Disabled
		: (Reduced() || SystemReduced())
		? Motion::Mode::Reduced
		: Motion::Mode::Full;
}

bool Reduced() {
	return Core::App().settings().readPref<bool>("fork/reduced-motion", false);
}

void SetReduced(bool reduced) {
	Core::App().settings().writePref<bool>("fork/reduced-motion", reduced);
	Core::App().saveSettingsDelayed();
}

} // namespace Ui::AuroraMotion
