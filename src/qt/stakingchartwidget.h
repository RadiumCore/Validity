// Copyright (c) 2025-2026 The Validity developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_STAKINGCHARTWIDGET_H
#define BITCOIN_QT_STAKINGCHARTWIDGET_H

#include "amount.h"

#include <QWidget>
#include <QVector>
#include <QString>
#include <QPixmap>

struct StakeDayData {
    QString label;     // e.g. "Feb 25"
    CAmount amount;    // satoshis
};

class StakingChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StakingChartWidget(QWidget *parent = 0);

    void setData(const QVector<StakeDayData> &data);
    void setUnit(int unit);

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void leaveEvent(QEvent *event);

private:
    QVector<StakeDayData> chartData;
    int displayUnit;
    int hoveredBar;

    // Paint cache — avoids full repaint on every frame
    QPixmap paintCache;
    bool cacheDirty;
    int cachedHoverBar;
    void rebuildCache();

    QRect getBarRect(int index, int chartLeft, int chartTop, int chartWidth, int chartHeight, CAmount maxAmount) const;
};

#endif // BITCOIN_QT_STAKINGCHARTWIDGET_H
