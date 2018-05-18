#include <qapplication.h>
#include <GLWidget.h>
#include <Engine.h>
#include <iostream>

int main(int argc, char* argv[])
{

#ifdef _WINDOWS
#ifdef _DEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif 
#endif
 // try
  //{
    QSurfaceFormat format;
    format.setSwapInterval(0);
    QSurfaceFormat::setDefaultFormat(format);
    QApplication app(argc, argv);
    GLWidget gl_widget;
    gl_widget.resize(1024, 768);
    gl_widget.showFullScreen();

    return app.exec();
  /*}
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    getchar();
    return -1;
  }*/
}