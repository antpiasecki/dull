#pragma once

#include "ui_mainwindow.h"
#include "vault.h"
#include <QMainWindow>
#include <memory>

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);

private:
  Ui::MainWindow *ui;

  std::unique_ptr<Vault> m_vault;

  void reload_fs_tree();
  void preview_file(const std::string &filename);
  void edit_file(const std::string &filename);
  void file_context_menu(const QPoint &pos);
};
