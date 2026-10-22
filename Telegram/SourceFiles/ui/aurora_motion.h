#pragma once

#include "ui/effects/motion_model.h"

namespace Ui::AuroraMotion {

[[nodiscard]] Motion::Mode Mode();
[[nodiscard]] bool Reduced();
void SetReduced(bool reduced);

} // namespace Ui::AuroraMotion
