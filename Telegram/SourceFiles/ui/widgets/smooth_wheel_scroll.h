#pragma once

#include "base/basic_types.h"

class QWidget;

namespace Ui {

class RpWidget;
class ScrollArea;
class ElasticScroll;

// Owns the custom wheel hook; install where no other wheel policy is set.
void SetupSmoothWheelScroll(not_null<ScrollArea*> scroll);
void SetupSmoothWheelScroll(not_null<ElasticScroll*> scroll);
void ResetSmoothWheelScroll(not_null<QWidget*> scroll);

} // namespace Ui
