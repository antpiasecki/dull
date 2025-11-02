#pragma once

#include "ui_mainwindow.h"
#include <QMainWindow>

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);

private:
  Ui::MainWindow *ui;

  std::string m_vault_path;

  void reload_vault();
  void preview_file(const std::string &filename);
};
