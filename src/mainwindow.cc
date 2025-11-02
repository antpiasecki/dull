#include "mainwindow.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QTemporaryFile>
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

    m_vault = Vault(path.toStdString());
    reload_fs_tree();
  });

  connect(ui->actionOpen, &QAction::triggered, this, [this]() {
    QString path = QFileDialog::getOpenFileName(this, "Choose vault to open",
                                                QDir::currentPath(),
                                                "Dull Vaults (*.dull)");
    if (path.isEmpty()) {
      return;
    }

    m_vault = Vault(path.toStdString());
    reload_fs_tree();
  });

  connect(ui->fsTreeWidget, &QTreeWidget::itemClicked,
          [this](QTreeWidgetItem *item, int column) {
            preview_file(item->text(column).toStdString());
          });

  connect(ui->fsTreeWidget, &QTreeWidget::customContextMenuRequested, this,
          &MainWindow::file_context_menu);

  connect(ui->actionAddFile, &QAction::triggered, this, [this]() {
    if (!m_vault) {
      return;
    }

    QString path = QFileDialog::getOpenFileName(this, "Choose a file to add");
    if (path.isEmpty()) {
      return;
    }

    std::ifstream file(path.toStdString(), std::ios::binary);

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    m_vault->write_file(path_to_filename(path.toStdString()), content);

    reload_fs_tree();
  });

  connect(ui->actionCreateFile, &QAction::triggered, this, [this]() {
    if (!m_vault) {
      return;
    }

    QString path =
        QInputDialog::getText(this, "Create File", "Where to create the file");
    if (path.isEmpty()) {
      return;
    }

    m_vault->write_file(path.toStdString(), "");

    reload_fs_tree();
  });
}

void MainWindow::reload_fs_tree() {
  ui->fsTreeWidget->clear();

  auto headers = m_vault->read_file_headers();
  for (const auto &header : headers) {
    auto *item = new QTreeWidgetItem(ui->fsTreeWidget);
    item->setText(0, QString::fromStdString(header.name));
  }
}

void MainWindow::preview_file(const std::string &filename) {
  auto content = m_vault->read_file(filename);
  if (content) {
    ui->filePreview->setText(QString::fromStdString(content.value()));
  } else {
    qWarning() << "File to preview not found";
  }
}

void MainWindow::edit_file(const std::string &filename) {
  auto content = m_vault->read_file(filename);
  if (content) {
    QTemporaryFile temp_file;
    ASSERT(temp_file.open());
    {
      QTextStream out(&temp_file);
      out << QString::fromStdString(content.value());
    }
    temp_file.flush();

    // TODO: xdg-open or something
    std::system(("kwrite " + temp_file.fileName()).toStdString().c_str());
    // TODO: write back
  } else {
    qWarning() << "File to edit not found";
  }
}

void MainWindow::file_context_menu(const QPoint &pos) {
  QTreeWidgetItem *item = ui->fsTreeWidget->itemAt(pos);
  if (item == nullptr) {
    return;
  }

  QMenu menu(this);

  QAction *edit_action = menu.addAction(
      style()->standardIcon(QStyle::SP_FileDialogDetailedView), "Edit");
  connect(edit_action, &QAction::triggered, this,
          [this, item]() { edit_file(item->text(0).toStdString()); });

  menu.exec(ui->fsTreeWidget->mapToGlobal(pos));
}
