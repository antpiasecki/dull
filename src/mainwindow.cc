#include "mainwindow.h"
#include "vault.h"
#include <QDebug>
#include <QFileDialog>
#include <QTreeWidgetItem>
#include <fstream>

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
      Vault::write_header(file);

      Vault::write_file(file, "hello.txt", "Hello, World!");
      Vault::write_file(file, "test.txt", "test test test");
    }

    m_vault_path = path.toStdString();
    reload_vault();
  });

  connect(ui->actionOpen, &QAction::triggered, this, [this]() {
    QString path = QFileDialog::getOpenFileName(this, "Choose vault to open",
                                                QDir::currentPath(),
                                                "Dull Vaults (*.dull)");
    if (path.isEmpty()) {
      return;
    }

    m_vault_path = path.toStdString();
    reload_vault();
  });

  connect(ui->fsTreeWidget, &QTreeWidget::itemClicked,
          [this](QTreeWidgetItem *item, int column) {
            preview_file(item->text(column).toStdString());
          });
}

void MainWindow::reload_vault() {
  ui->fsTreeWidget->clear();

  std::ifstream file(m_vault_path, std::ios::binary);
  Vault::verify_header(file);

  while (true) {
    auto header = Vault::read_file_header(file);
    if (!header) {
      break;
    }
    file.seekg(static_cast<i64>(header->size), std::ios::cur);

    auto *item = new QTreeWidgetItem(ui->fsTreeWidget);
    item->setText(0, QString::fromStdString(header->name));
  }
}

void MainWindow::preview_file(const std::string &filename) {
  std::ifstream file(m_vault_path, std::ios::binary);
  Vault::verify_header(file);

  while (true) {
    auto header = Vault::read_file_header(file);
    if (!header) {
      break;
    }

    if (header->name == filename) {
      std::string content(header->size, '\0');
      file.read(content.data(), static_cast<i64>(header->size));
      ui->filePreview->setText(QString::fromStdString(content));
      return;
    }
    file.seekg(static_cast<i64>(header->size), std::ios::cur);
  }

  ASSERT(false);
}
