#include "mainwindow.h"
#include <QApplication>

i32 main(i32 argc, char *argv[]) {
  QApplication app(argc, argv);

  MainWindow window;
  window.show();

  return QApplication::exec();
}
