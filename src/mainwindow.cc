#include "mainwindow.h"
#include "common.h"
#include <QDebug>
#include <QFileDialog>
#include <QTreeWidgetItem>
#include <fstream>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  connect(ui->actionNew, &QAction::triggered, this, [this]() {
    QString path = QFileDialog::getSaveFileName(this, "Choose vault location",
                                                QDir::currentPath(),
                                                "Dull Vaults (*.dull)");
    if (path.isEmpty()) {
      return;
    }

    {
      std::ofstream file(path.toStdString(), std::ios::binary);
      file.write("DULL", 4);

      std::string file_name = "hello.txt";
      u64 file_name_size = file_name.size();
      std::string file_content = "Hello, World!";
      u64 file_size = file_content.size();
      file.write(reinterpret_cast<const char *>(&file_name_size), sizeof(u64));
      file.write(file_name.data(), static_cast<i64>(file_name_size));
      file.write(reinterpret_cast<const char *>(&file_size), sizeof(u64));
      file.write(file_content.data(), static_cast<i64>(file_size));
    }

    m_vault_path = path;
    reload_vault();
  });

  connect(ui->actionOpen, &QAction::triggered, this, [this]() {
    QString path = QFileDialog::getOpenFileName(this, "Choose vault to open",
                                                QDir::currentPath(),
                                                "Dull Vaults (*.dull)");
    if (path.isEmpty()) {
      return;
    }

    m_vault_path = path;
    reload_vault();
  });
}

void MainWindow::reload_vault() {
  ui->fsTreeWidget->clear();

  std::ifstream file(m_vault_path.toStdString(), std::ios::binary);
  ASSERT(file.good());

  std::array<i8, 4> header{};
  ASSERT(file.read(header.data(), header.size()));

  ASSERT(std::string_view(reinterpret_cast<i8 *>(header.data()),
                          header.size()) == "DULL");

  while (true) {
    u64 name_size = 0;
    if (!file.read(reinterpret_cast<char *>(&name_size), sizeof(u64))) {
      break;
    }

    std::string name(name_size, '\0');
    file.read(name.data(), static_cast<i64>(name_size));

    u64 content_size = 0;
    file.read(reinterpret_cast<char *>(&content_size), sizeof(u64));

    std::string content(content_size, '\0');
    file.read(content.data(), static_cast<i64>(content_size));

    auto *item = new QTreeWidgetItem(ui->fsTreeWidget);
    item->setText(0, QString::fromStdString(name));
  }
}
