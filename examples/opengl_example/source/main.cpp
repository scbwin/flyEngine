#include <qapplication.h>
#include "GLWidget.h"

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);

  QSurfaceFormat format;
  format.setSwapInterval(0);
  QSurfaceFormat::setDefaultFormat(format);
  
  GLWidget widget;

  widget.resize(1024, 768);
  widget.show();

  return app.exec();
}