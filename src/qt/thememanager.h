// Copyright (c) 2025 The Validity developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_THEMEMANAGER_H
#define BITCOIN_QT_THEMEMANAGER_H

#include <QObject>
#include <QString>

class QApplication;

class ThemeManager : public QObject
{
    Q_OBJECT

public:
    enum Theme {
        Light = 0,
        Dark = 1
    };

    explicit ThemeManager(QObject *parent = 0);

    /** Apply a theme to the application */
    void setTheme(Theme theme);

    /** Get the currently active theme */
    Theme currentTheme() const { return activeTheme; }

    /** Load saved theme preference from QSettings */
    void loadSavedTheme();

    /** Check if dark theme is active */
    bool isDark() const { return activeTheme == Dark; }

    /** Get list of available theme names for UI */
    static QStringList availableThemes();

    /** Convert theme enum to display name */
    static QString themeName(Theme theme);

Q_SIGNALS:
    void themeChanged(int theme);

private:
    Theme activeTheme;

    /** Load QSS content from resource file */
    QString loadStyleSheet(const QString &filename) const;
};

#endif // BITCOIN_QT_THEMEMANAGER_H
