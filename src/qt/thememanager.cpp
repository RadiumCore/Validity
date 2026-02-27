// Copyright (c) 2025 The Validity developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "thememanager.h"

#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QSettings>
#include <QStringList>

ThemeManager::ThemeManager(QObject *parent) :
    QObject(parent),
    activeTheme(Dark)
{
}

void ThemeManager::setTheme(Theme theme)
{
    activeTheme = theme;

    QString qss;
    if (theme == Dark) {
        qss = loadStyleSheet(":/themes/dark");
    } else {
        qss = loadStyleSheet(":/themes/light");
    }

    qApp->setStyleSheet(qss);

    // Update application palette to match theme (for PlatformStyle and palette-aware code)
    QPalette pal = qApp->palette();
    if (theme == Dark) {
        pal.setColor(QPalette::Window, QColor(15, 15, 36));         // #0f0f24
        pal.setColor(QPalette::WindowText, QColor(232, 232, 240));  // #e8e8f0
        pal.setColor(QPalette::Base, QColor(10, 10, 26));           // #0a0a1a
        pal.setColor(QPalette::AlternateBase, QColor(17, 17, 40));  // #111128
        pal.setColor(QPalette::Text, QColor(232, 232, 240));        // #e8e8f0
        pal.setColor(QPalette::Button, QColor(26, 26, 58));         // #1a1a3a
        pal.setColor(QPalette::ButtonText, QColor(232, 232, 240));  // #e8e8f0
        pal.setColor(QPalette::Highlight, QColor(67, 181, 129));    // #43b581
        pal.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
        pal.setColor(QPalette::ToolTipBase, QColor(21, 21, 48));    // #151530
        pal.setColor(QPalette::ToolTipText, QColor(232, 232, 240)); // #e8e8f0
        pal.setColor(QPalette::Disabled, QPalette::Text, QColor(90, 90, 122));     // #5a5a7a
        pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(90, 90, 122));
    } else {
        // Reset to default system palette for light theme
        pal = QPalette();
    }
    qApp->setPalette(pal);

    // Save preference
    QSettings settings;
    settings.setValue("nTheme", static_cast<int>(theme));

    Q_EMIT themeChanged(static_cast<int>(theme));
}

void ThemeManager::loadSavedTheme()
{
    QSettings settings;
    int savedTheme = settings.value("nTheme", static_cast<int>(Dark)).toInt();
    if (savedTheme == Light || savedTheme == Dark) {
        setTheme(static_cast<Theme>(savedTheme));
    } else {
        setTheme(Dark);
    }
}

QString ThemeManager::loadStyleSheet(const QString &filename) const
{
    QFile file(filename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        return QString();
    }
    return file.readAll();
}

QStringList ThemeManager::availableThemes()
{
    return QStringList() << themeName(Light) << themeName(Dark);
}

QString ThemeManager::themeName(Theme theme)
{
    switch (theme) {
        case Light: return "Light";
        case Dark: return "Dark";
        default: return "Dark";
    }
}
