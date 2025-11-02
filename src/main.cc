#include "ui_mainwindow.h"
#include <QApplication>
#include <QMainWindow>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  QMainWindow window;
  Ui::MainWindow ui;
  ui.setupUi(&window);

  window.show();

  return QApplication::exec();
}
