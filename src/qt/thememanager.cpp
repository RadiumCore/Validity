// Copyright (c) 2025 The Validity developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "thememanager.h"

#include <QApplication>
#include <QFile>
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
