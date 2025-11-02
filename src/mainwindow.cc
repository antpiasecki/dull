#include "mainwindow.h"
#include <QDesktopServices>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QTemporaryFile>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(std::make_unique<Ui::MainWindow>()) {
  ui->setupUi(this);

  ui->filePreview->setVisible(false);

  connect(ui->actionNew, &QAction::triggered, this, [this]() {
    QString path = QFileDialog::getSaveFileName(this, "Choose vault location",
                                                QDir::currentPath(),
                                                "Dull Vaults (*.dull)");
    if (path.isEmpty()) {
      return;
    }

    m_vault = std::make_unique<Vault>(path.toStdString());
    reload_fs_tree();
  });

  connect(ui->actionOpen, &QAction::triggered, this, [this]() {
    QString path = QFileDialog::getOpenFileName(this, "Choose vault to open",
                                                QDir::currentPath(),
                                                "Dull Vaults (*.dull)");
    if (path.isEmpty()) {
      return;
    }

    m_vault = std::make_unique<Vault>(path.toStdString());
    reload_fs_tree();
  });

  connect(
      ui->fsTreeWidget, &QTreeWidget::itemClicked,
      [this](QTreeWidgetItem *, int) { ui->filePreview->setVisible(false); });

  connect(ui->fsTreeWidget, &QTreeWidget::customContextMenuRequested, this,
          &MainWindow::file_context_menu);

  connect(ui->actionAddFiles, &QAction::triggered, this, [this]() {
    if (!m_vault) {
      return;
    }

    QStringList paths =
        QFileDialog::getOpenFileNames(this, "Choose files to add");
    for (const auto &path : paths) {
      std::ifstream file(path.toStdString(), std::ios::binary);

      std::string content((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());

      m_vault->create_file(path_to_filename(path.toStdString()), content);
    }

    reload_fs_tree();
  });
}

void MainWindow::reload_fs_tree() {
  ui->menuFiles->setEnabled(true);
  ui->fsTreeWidget->clear();

  auto headers = m_vault->read_file_headers();
  for (const auto &header : headers) {
    auto *item = new QTreeWidgetItem(ui->fsTreeWidget);
    item->setText(0, QString::fromStdString(header.name));
    item->setText(1, QString::number(header.size));
    item->setText(2, QString::number(header.offset));
  }
  ui->fsTreeWidget->resizeColumnToContents(0);
}

void MainWindow::preview_file(const std::string &filename) {
  ui->filePreview->setVisible(true);

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

    QDesktopServices::openUrl(QUrl::fromLocalFile(temp_file.fileName()));
    QMessageBox::information(this, "Edit",
                             "Please edit the file in the opened editor and "
                             "save it. Click OK when done.");

    temp_file.seek(0);
    QTextStream in(&temp_file);
    m_vault->update_file(filename, in.readAll().toStdString());
    reload_fs_tree();

    // QTemporaryFile gets deleted when it goes out of scope
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

  QAction *preview_action = menu.addAction(
      style()->standardIcon(QStyle::SP_FileDialogContentsView), "Preview");
  connect(preview_action, &QAction::triggered, this,
          [this, item]() { preview_file(item->text(0).toStdString()); });

  QAction *edit_action = menu.addAction(
      style()->standardIcon(QStyle::SP_FileDialogDetailedView), "Edit");
  connect(edit_action, &QAction::triggered, this,
          [this, item]() { edit_file(item->text(0).toStdString()); });

  menu.exec(ui->fsTreeWidget->mapToGlobal(pos));
}
