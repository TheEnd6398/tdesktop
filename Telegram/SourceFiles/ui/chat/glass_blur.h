#pragma once

#include <QtGui/QImage>

namespace Ui::Glass {

void Blur(QImage &image, QImage &scratch, int radius = 3);

} // namespace Ui::Glass
