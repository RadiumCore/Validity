// Copyright (c) 2026 The Validity developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_BACKUPWIZARD_H
#define BITCOIN_QT_BACKUPWIZARD_H

#include <QDialog>

class WalletModel;

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QPushButton;
class QProgressBar;
class QStackedWidget;
QT_END_NAMESPACE

class BackupWizard : public QDialog
{
    Q_OBJECT

public:
    explicit BackupWizard(WalletModel *walletModel, QWidget *parent = 0);

private Q_SLOTS:
    void nextPage();
    void prevPage();
    void browseClicked();
    void performBackup();

private:
    void setupIntroPage();
    void setupLocationPage();
    void setupProgressPage();
    void setupCompletePage();
    void updateButtons();

    WalletModel *walletModel;
    QStackedWidget *stack;

    // Navigation buttons
    QPushButton *backButton;
    QPushButton *nextButton;
    QPushButton *cancelButton;

    // Location page widgets
    QLineEdit *pathEdit;

    // Progress page widgets
    QProgressBar *progressBar;
    QLabel *statusLabel;

    // Complete page widgets
    QLabel *resultLabel;
    QLabel *pathResultLabel;

    int currentPage;
    bool backupSuccess;
};

#endif // BITCOIN_QT_BACKUPWIZARD_H
