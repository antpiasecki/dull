#include "mainwindow.h"
#include <QDesktopServices>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QTemporaryDir>
#include <botan/auto_rng.h>
#include <botan/hex.h>

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

    if (!path.contains(".dull")) {
      path += ".dull";
    }

    QString password = QInputDialog::getText(
        this, "Choose a password", "Choose a password", QLineEdit::Password);
    if (password.length() < 8) {
      QMessageBox::critical(this, "Error",
                            "Password must be at least 8 characters long.");
      return;
    }

    static Botan::AutoSeeded_RNG rng;
    auto salt_sv = rng.random_vec(16);
    std::vector<u8> salt(salt_sv.begin(), salt_sv.end());

    std::ofstream create(path.toStdString(), std::ios::binary);
    create.write("DULL", 4);
    create.write(to_char_ptr(&VERSION), sizeof(VERSION));
    create.write(to_char_ptr(salt.data()), 16);
    create.close();

    m_vault =
        std::make_unique<Vault>(path.toStdString(), password.toStdString());
    reload_fs_tree();
  });

  connect(ui->actionOpen, &QAction::triggered, this, [this]() {
    QString path = QFileDialog::getOpenFileName(this, "Choose vault to open",
                                                QDir::currentPath(),
                                                "Dull Vaults (*.dull)");
    if (path.isEmpty()) {
      return;
    }

    QString password = QInputDialog::getText(
        this, "Unlock the vault", "Enter vault password", QLineEdit::Password);

    // TODO: check if password valid

    m_vault =
        std::make_unique<Vault>(path.toStdString(), password.toStdString());
    reload_fs_tree();
  });

  connect(
      ui->fsTreeWidget, &QTreeWidget::itemClicked,
      [this](QTreeWidgetItem *, i32) { ui->filePreview->setVisible(false); });

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
    item->setText(1, QString::number(header.content_size));
    item->setText(2, QString::number(header.global_offset));
    item->setText(3, QString::fromStdString(Botan::hex_encode(header.nonce)));
  }
  for (int i = 0; i < ui->fsTreeWidget->columnCount(); ++i) {
    ui->fsTreeWidget->resizeColumnToContents(i);
  }
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

void MainWindow::extract_file(const std::string &filename) {
  auto content = m_vault->read_file(filename);
  if (content) {
    QString path = QFileDialog::getSaveFileName(
        this, "Choose location to extract",
        QDir::currentPath() + "/" + QString::fromStdString(filename));
    if (path.isEmpty()) {
      return;
    }

    std::ofstream file(path.toStdString(), std::ios::binary);
    file.write(content->data(), static_cast<i64>(content->size()));
  } else {
    qWarning() << "File to extract not found";
  }
}

void MainWindow::edit_file(const std::string &filename) {
  auto content = m_vault->read_file(filename);
  if (content) {
    QTemporaryDir dir;
    ASSERT(dir.isValid());

    std::string path = dir.path().toStdString() + "/" + filename;

    std::ofstream file(path);
    file.write(content->data(), static_cast<i64>(content->size()));
    file.close();

    QDesktopServices::openUrl(
        QUrl::fromLocalFile(QString::fromStdString(path)));
    QMessageBox::information(this, "Edit",
                             "Please edit the file in the opened editor and "
                             "save it. Click OK when done.");

    std::ifstream file2(path, std::ios::binary);
    std::string new_content((std::istreambuf_iterator<char>(file2)),
                            std::istreambuf_iterator<char>());

    m_vault->update_file(filename, new_content);
    reload_fs_tree();

    // QTemporaryDir gets deleted when it goes out of scope
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

  QAction *extract_action =
      menu.addAction(style()->standardIcon(QStyle::SP_DriveHDIcon), "Extract");
  connect(extract_action, &QAction::triggered, this,
          [this, item]() { extract_file(item->text(0).toStdString()); });

  QAction *delete_action = menu.addAction(
      style()->standardIcon(QStyle::SP_DialogCancelButton), "Delete");
  connect(delete_action, &QAction::triggered, this, [this, item]() {
    m_vault->delete_file(item->text(0).toStdString());
    reload_fs_tree();
  });

  menu.exec(ui->fsTreeWidget->mapToGlobal(pos));
}
