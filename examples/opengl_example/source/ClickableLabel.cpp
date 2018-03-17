#include "ClickableLabel.h"
#include <qevent.h>

ClickableLabel::ClickableLabel(const QString& text) : QLabel(text)
{
}

void ClickableLabel::mousePressEvent(QMouseEvent * event)
{
}

void ClickableLabel::mouseReleaseEvent(QMouseEvent * event)
{
  emit clicked();
}
