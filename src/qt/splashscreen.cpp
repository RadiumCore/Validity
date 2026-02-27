// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2025-2026 The Validity developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "splashscreen.h"

#include "networkstyle.h"

#include "clientversion.h"
#include "init.h"
#include "util.h"
#include "ui_interface.h"
#include "version.h"

#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif

#include <QApplication>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QPainter>
#include <QLinearGradient>
#include <QRadialGradient>

SplashScreen::SplashScreen(Qt::WindowFlags f, const NetworkStyle *networkStyle) :
    QWidget(0, f), curAlignment(0)
{
    float devicePixelRatio = 1.0;
#if QT_VERSION > 0x050100
    devicePixelRatio = ((QGuiApplication*)QCoreApplication::instance())->devicePixelRatio();
#endif

    // Create splash bitmap
    QSize splashSize(520*devicePixelRatio, 340*devicePixelRatio);
    pixmap = QPixmap(splashSize);

#if QT_VERSION > 0x050100
    pixmap.setDevicePixelRatio(devicePixelRatio);
#endif

    QPainter pixPaint(&pixmap);
    pixPaint.setRenderHint(QPainter::Antialiasing, true);
    pixPaint.setRenderHint(QPainter::TextAntialiasing, true);

    int w = splashSize.width() / devicePixelRatio;
    int h = splashSize.height() / devicePixelRatio;

    // === Dark cosmic gradient background ===
    QLinearGradient bgGradient(0, 0, 0, h);
    bgGradient.setColorAt(0.0, QColor(10, 10, 26));    // #0a0a1a deep navy
    bgGradient.setColorAt(0.5, QColor(15, 15, 36));     // #0f0f24
    bgGradient.setColorAt(1.0, QColor(26, 16, 48));     // #1a1030 dark purple
    pixPaint.fillRect(QRect(0, 0, w, h), bgGradient);

    // === Subtle radial glow in center ===
    QRadialGradient centerGlow(w/2, h/2 - 30, 180);
    centerGlow.setColorAt(0.0, QColor(67, 181, 129, 15));  // very faint green
    centerGlow.setColorAt(1.0, QColor(67, 181, 129, 0));
    pixPaint.fillRect(QRect(0, 0, w, h), centerGlow);

    // === Draw the app icon (Validity diamond) ===
    const QSize requiredSize(256, 256);
    QPixmap icon(networkStyle->getAppIcon().pixmap(requiredSize));
    int iconSize = 80;
    int iconX = (w - iconSize) / 2;
    int iconY = 40;
    pixPaint.drawPixmap(QRect(iconX, iconY, iconSize, iconSize), icon);

    // === Title: "VAL" in white + "IDITY" in green ===
    QString font = QApplication::font().toString();
    QFont titleFont(font, 28);
    titleFont.setWeight(QFont::Bold);
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 4);
    pixPaint.setFont(titleFont);

    QString titlePart1 = "VAL";
    QString titlePart2 = "IDITY";
    QFontMetrics fm = pixPaint.fontMetrics();
    int totalWidth = fm.width(titlePart1) + fm.width(titlePart2);
    int titleX = (w - totalWidth) / 2;
    int titleY = iconY + iconSize + 40;

    pixPaint.setPen(QColor(232, 232, 240));  // #e8e8f0 white
    pixPaint.drawText(titleX, titleY, titlePart1);

    pixPaint.setPen(QColor(67, 181, 129));   // #43b581 green
    pixPaint.drawText(titleX + fm.width(titlePart1), titleY, titlePart2);

    // === Version text ===
    QString versionText = QString("v%1").arg(QString::fromStdString(FormatFullVersion()));
    QFont versionFont(font, 11);
    pixPaint.setFont(versionFont);
    pixPaint.setPen(QColor(136, 136, 168));  // #8888a8 muted
    fm = pixPaint.fontMetrics();
    int versionX = (w - fm.width(versionText)) / 2;
    pixPaint.drawText(versionX, titleY + 24, versionText);

    // === Copyright text ===
    QFont copyrightFont(font, 8);
    pixPaint.setFont(copyrightFont);
    pixPaint.setPen(QColor(90, 90, 122));    // #5a5a7a dim
    fm = pixPaint.fontMetrics();

    QStringList copyrightLines;
    copyrightLines << QChar(0xA9) + QString(" 2009-%1 The Bitcoin Core developers").arg(COPYRIGHT_YEAR);
    copyrightLines << QChar(0xA9) + QString(" 2014-2018 The Blackcoin developers");
    copyrightLines << QChar(0xA9) + QString(" 2018-%1 The Blackcoin More developers").arg(COPYRIGHT_YEAR);
    copyrightLines << QChar(0xA9) + QString(" 2019-%1 The Radium Core developers").arg(COPYRIGHT_YEAR);
    copyrightLines << QChar(0xA9) + QString(" 2020-%1 The Validity developers").arg(COPYRIGHT_YEAR);

    int lineHeight = fm.height() + 2;
    int copyrightY = titleY + 40;
    for (int i = 0; i < copyrightLines.size(); i++) {
        int cx = (w - fm.width(copyrightLines[i])) / 2;
        pixPaint.drawText(cx, copyrightY + i * lineHeight, copyrightLines[i]);
    }

    // === Bottom accent line ===
    QLinearGradient lineGradient(w * 0.2, 0, w * 0.8, 0);
    lineGradient.setColorAt(0.0, QColor(67, 181, 129, 0));
    lineGradient.setColorAt(0.5, QColor(67, 181, 129, 180));
    lineGradient.setColorAt(1.0, QColor(67, 181, 129, 0));
    pixPaint.setPen(Qt::NoPen);
    pixPaint.setBrush(lineGradient);
    pixPaint.drawRect(QRectF(w * 0.2, h - 60, w * 0.6, 1));

    // === Network name for testnet ===
    QString titleAddText = networkStyle->getTitleAddText();
    if(!titleAddText.isEmpty()) {
        QFont boldFont(font, 10);
        boldFont.setWeight(QFont::Bold);
        pixPaint.setFont(boldFont);
        pixPaint.setPen(QColor(67, 181, 129));
        fm = pixPaint.fontMetrics();
        int addX = (w - fm.width(titleAddText)) / 2;
        pixPaint.drawText(addX, titleY + 72, titleAddText);
    }

    pixPaint.end();

    // Set window title
    QString titleText = tr(PACKAGE_NAME);
    setWindowTitle(titleText + " " + titleAddText);

    // Resize and center
    QRect r(QPoint(), QSize(pixmap.size().width()/devicePixelRatio, pixmap.size().height()/devicePixelRatio));
    resize(r.size());
    setFixedSize(r.size());
    move(QApplication::desktop()->screenGeometry().center() - r.center());

    subscribeToCoreSignals();
}

SplashScreen::~SplashScreen()
{
    unsubscribeFromCoreSignals();
}

void SplashScreen::slotFinish(QWidget *mainWin)
{
    Q_UNUSED(mainWin);
    if (isMinimized())
        showNormal();
    hide();
    deleteLater();
}

static void InitMessage(SplashScreen *splash, const std::string &message)
{
    QMetaObject::invokeMethod(splash, "showMessage",
        Qt::QueuedConnection,
        Q_ARG(QString, QString::fromStdString(message)),
        Q_ARG(int, Qt::AlignBottom|Qt::AlignHCenter),
        Q_ARG(QColor, QColor(136, 136, 168)));  // #8888a8 muted text
}

static void ShowProgress(SplashScreen *splash, const std::string &title, int nProgress)
{
    InitMessage(splash, title + strprintf("%d", nProgress) + "%");
}

#ifdef ENABLE_WALLET
static void ConnectWallet(SplashScreen *splash, CWallet* wallet)
{
    wallet->ShowProgress.connect(boost::bind(ShowProgress, splash, _1, _2));
}
#endif

void SplashScreen::subscribeToCoreSignals()
{
    uiInterface.InitMessage.connect(boost::bind(InitMessage, this, _1));
    uiInterface.ShowProgress.connect(boost::bind(ShowProgress, this, _1, _2));
#ifdef ENABLE_WALLET
    uiInterface.LoadWallet.connect(boost::bind(ConnectWallet, this, _1));
#endif
}

void SplashScreen::unsubscribeFromCoreSignals()
{
    uiInterface.InitMessage.disconnect(boost::bind(InitMessage, this, _1));
    uiInterface.ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
#ifdef ENABLE_WALLET
    if(pwalletMain)
        pwalletMain->ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
#endif
}

void SplashScreen::showMessage(const QString &message, int alignment, const QColor &color)
{
    curMessage = message;
    curAlignment = alignment;
    curColor = color;
    update();
}

void SplashScreen::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, pixmap);

    // Draw loading message at the bottom
    QRect r = rect().adjusted(20, 5, -20, -12);
    painter.setPen(curColor);
    QFont msgFont = painter.font();
    msgFont.setPixelSize(11);
    painter.setFont(msgFont);
    painter.drawText(r, curAlignment, curMessage);
}

void SplashScreen::closeEvent(QCloseEvent *event)
{
    StartShutdown();
    event->ignore();
}
