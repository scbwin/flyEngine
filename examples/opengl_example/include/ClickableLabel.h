#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>

class ClickableLabel : public QLabel
{
  Q_OBJECT
public:
  ClickableLabel(const QString& text);
signals:
  void clicked();
protected:
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;

};

#endif // !CLICKABLELABEL_H
