// Copyright (c) 2026 The Validity developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "backupwizard.h"
#include "walletmodel.h"
#include "guiutil.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QStackedWidget>
#include <QFileDialog>
#include <QStandardPaths>
#include <QFileInfo>
#include <QApplication>

BackupWizard::BackupWizard(WalletModel *_walletModel, QWidget *parent)
    : QDialog(parent), walletModel(_walletModel), currentPage(0), backupSuccess(false)
{
    setWindowTitle(tr("Backup Wallet"));
    setMinimumSize(520, 420);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Stacked widget for pages
    stack = new QStackedWidget;
    mainLayout->addWidget(stack, 1);

    // Navigation buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    backButton = new QPushButton(tr("< Back"));
    nextButton = new QPushButton(tr("Next >"));
    cancelButton = new QPushButton(tr("Cancel"));

    buttonLayout->addWidget(backButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(nextButton);
    mainLayout->addLayout(buttonLayout);

    connect(backButton, SIGNAL(clicked()), this, SLOT(prevPage()));
    connect(nextButton, SIGNAL(clicked()), this, SLOT(nextPage()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    // Create pages
    setupIntroPage();
    setupLocationPage();
    setupProgressPage();
    setupCompletePage();

    updateButtons();
}

void BackupWizard::setupIntroPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);

    QLabel *title = new QLabel(tr("Backup Your Wallet"));
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #43b581;");

    QLabel *subtitle = new QLabel(tr("Create a secure copy of your wallet data."));
    subtitle->setStyleSheet("color: #8888a8; font-size: 13px; margin-bottom: 16px;");

    QLabel *body = new QLabel(tr(
        "This wizard will help you create a backup of your wallet.\n\n"
        "Your wallet file contains your private keys, transaction history, "
        "and address labels. If this file is lost or corrupted, you will "
        "lose access to your funds.\n\n"
        "It is recommended to:\n"
        "  \342\200\242 Back up to an external drive or cloud storage\n"
        "  \342\200\242 Keep multiple copies in different locations\n"
        "  \342\200\242 Back up after every 100 transactions\n\n"
        "Click Next to choose a backup location."
    ));
    body->setWordWrap(true);
    body->setStyleSheet("font-size: 13px; line-height: 1.5;");

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(body);
    layout->addStretch();

    stack->addWidget(page);
}

void BackupWizard::setupLocationPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);

    QLabel *title = new QLabel(tr("Choose Backup Location"));
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #43b581;");

    QLabel *subtitle = new QLabel(tr("Select where to save the backup file."));
    subtitle->setStyleSheet("color: #8888a8; font-size: 13px; margin-bottom: 16px;");

    pathEdit = new QLineEdit;
    pathEdit->setPlaceholderText(tr("Click Browse to choose a location..."));
    pathEdit->setReadOnly(true);

    QPushButton *browseButton = new QPushButton(tr("Browse..."));
    connect(browseButton, SIGNAL(clicked()), this, SLOT(browseClicked()));

    QHBoxLayout *pathLayout = new QHBoxLayout;
    pathLayout->addWidget(pathEdit);
    pathLayout->addWidget(browseButton);

    QLabel *hint = new QLabel(tr(
        "Tip: Choose an external drive, USB stick, or cloud-synced folder "
        "for maximum safety."
    ));
    hint->setWordWrap(true);
    hint->setStyleSheet("color: #8888a8; font-size: 11px; margin-top: 16px;");

    // Default path suggestion
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (!defaultPath.isEmpty()) {
        pathEdit->setText(defaultPath + "/validity-wallet-backup.dat");
    }

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addLayout(pathLayout);
    layout->addWidget(hint);
    layout->addStretch();

    stack->addWidget(page);
}

void BackupWizard::setupProgressPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);

    QLabel *title = new QLabel(tr("Creating Backup"));
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #43b581;");

    statusLabel = new QLabel(tr("Preparing backup..."));
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setWordWrap(true);
    statusLabel->setStyleSheet("font-size: 14px;");

    progressBar = new QProgressBar;
    progressBar->setRange(0, 0); // indeterminate

    layout->addWidget(title);
    layout->addStretch();
    layout->addWidget(statusLabel);
    layout->addSpacing(12);
    layout->addWidget(progressBar);
    layout->addStretch();

    stack->addWidget(page);
}

void BackupWizard::setupCompletePage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);

    QLabel *title = new QLabel(tr("Backup Complete"));
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #43b581;");

    resultLabel = new QLabel;
    resultLabel->setWordWrap(true);
    resultLabel->setStyleSheet("font-size: 13px;");

    pathResultLabel = new QLabel;
    pathResultLabel->setWordWrap(true);
    pathResultLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    layout->addWidget(title);
    layout->addSpacing(16);
    layout->addWidget(resultLabel);
    layout->addSpacing(12);
    layout->addWidget(pathResultLabel);
    layout->addStretch();

    stack->addWidget(page);
}

void BackupWizard::updateButtons()
{
    backButton->setEnabled(currentPage > 0 && currentPage < 3);
    backButton->setVisible(currentPage > 0 && currentPage < 3);

    if (currentPage == 3) {
        nextButton->setText(tr("Done"));
    } else {
        nextButton->setText(tr("Next >"));
    }

    cancelButton->setVisible(currentPage < 2);
}

void BackupWizard::nextPage()
{
    if (currentPage == 1 && pathEdit->text().isEmpty()) {
        return; // Don't proceed without a path
    }

    if (currentPage == 1) {
        // Moving to progress page - perform backup
        currentPage = 2;
        stack->setCurrentIndex(currentPage);
        updateButtons();
        QApplication::processEvents();
        performBackup();
        return;
    }

    if (currentPage == 3) {
        accept(); // Done
        return;
    }

    currentPage++;
    stack->setCurrentIndex(currentPage);
    updateButtons();
}

void BackupWizard::prevPage()
{
    if (currentPage > 0) {
        currentPage--;
        stack->setCurrentIndex(currentPage);
        updateButtons();
    }
}

void BackupWizard::browseClicked()
{
    QString filename = QFileDialog::getSaveFileName(this,
        tr("Backup Wallet"), pathEdit->text(),
        tr("Wallet Data (*.dat);;All Files (*)"));

    if (!filename.isEmpty()) {
        pathEdit->setText(filename);
    }
}

void BackupWizard::performBackup()
{
    QString backupPath = pathEdit->text();
    statusLabel->setText(tr("Backing up wallet to:\n%1").arg(backupPath));
    progressBar->setRange(0, 0);
    QApplication::processEvents();

    // Perform backup (synchronous - wallet.dat is small)
    backupSuccess = walletModel->backupWallet(backupPath);

    if (backupSuccess) {
        QFileInfo fi(backupPath);
        statusLabel->setText(tr("Backup successful!"));
        statusLabel->setStyleSheet("color: #43b581; font-weight: bold; font-size: 16px;");
        progressBar->setRange(0, 100);
        progressBar->setValue(100);
    } else {
        statusLabel->setText(tr("Backup failed!\n\nCould not write to:\n%1").arg(backupPath));
        statusLabel->setStyleSheet("color: #e05555; font-weight: bold; font-size: 14px;");
        progressBar->setRange(0, 100);
        progressBar->setValue(0);
    }

    // Auto-advance to complete page
    currentPage = 3;
    stack->setCurrentIndex(currentPage);

    if (backupSuccess) {
        resultLabel->setText(tr(
            "Your wallet has been successfully backed up!\n\n"
            "Remember to:\n"
            "  \342\200\242 Store this backup in a safe location\n"
            "  \342\200\242 Create backups regularly\n"
            "  \342\200\242 Never share your wallet file with anyone"
        ));
        resultLabel->setStyleSheet("font-size: 13px;");
        pathResultLabel->setText(tr("Backup saved to:\n%1").arg(backupPath));
        pathResultLabel->setStyleSheet("color: #43b581;");
    } else {
        resultLabel->setText(tr(
            "The backup could not be completed.\n\n"
            "Please try again with a different location."
        ));
        resultLabel->setStyleSheet("color: #e05555;");
        pathResultLabel->clear();
    }

    updateButtons();
}
