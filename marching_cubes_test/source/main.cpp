#include <iostream>
#include <MCWidget.h>

int main(int argc, char* argv[])
{
#ifdef _WINDOWS
#ifdef _DEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif 
#endif
  QApplication app(argc, argv);
  MCWidget mc_widget;
  mc_widget.resize(1024, 768);
  mc_widget.show();
  return app.exec();
}