// Copyright (c) 2025 The Validity developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "stakingchartwidget.h"
#include "bitcoinunits.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QToolTip>
#include <QResizeEvent>

static const int BAR_SPACING = 2;
static const int CHART_PADDING_LEFT = 60;
static const int CHART_PADDING_RIGHT = 10;
static const int CHART_PADDING_TOP = 10;
static const int CHART_PADDING_BOTTOM = 30;

static const QColor COLOR_BAR_NORMAL(67, 181, 129);        // #43b581 Validity green
static const QColor COLOR_BAR_HOVER(78, 204, 146);         // #4ecc92 brighter green on hover
static const QColor COLOR_BAR_ZERO(67, 181, 129, 30);      // very faint for zero-amount days
static const QColor COLOR_GRID(255, 255, 255, 12);         // ultra-subtle grid lines
static const QColor COLOR_AXIS_TEXT(136, 136, 168);         // #8888a8 muted text

StakingChartWidget::StakingChartWidget(QWidget *parent) :
    QWidget(parent),
    displayUnit(0),
    hoveredBar(-1),
    cacheDirty(true),
    cachedHoverBar(-1)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void StakingChartWidget::setData(const QVector<StakeDayData> &data)
{
    chartData = data;
    hoveredBar = -1;
    cacheDirty = true;
    update();
}

void StakingChartWidget::setUnit(int unit)
{
    displayUnit = unit;
    cacheDirty = true;
    update();
}

QSize StakingChartWidget::minimumSizeHint() const
{
    return QSize(200, 120);
}

QSize StakingChartWidget::sizeHint() const
{
    return QSize(400, 180);
}

void StakingChartWidget::resizeEvent(QResizeEvent *event)
{
    cacheDirty = true;
    QWidget::resizeEvent(event);
}

QRect StakingChartWidget::getBarRect(int index, int chartLeft, int chartTop, int chartWidth, int chartHeight, CAmount maxAmount) const
{
    if (chartData.isEmpty() || maxAmount <= 0)
        return QRect();

    int numBars = chartData.size();
    double barWidth = (double)(chartWidth - (numBars - 1) * BAR_SPACING) / numBars;
    if (barWidth < 1) barWidth = 1;

    int x = chartLeft + (int)(index * (barWidth + BAR_SPACING));
    double ratio = (double)chartData[index].amount / (double)maxAmount;
    int barHeight = (int)(ratio * chartHeight);
    if (chartData[index].amount > 0 && barHeight < 2)
        barHeight = 2; // minimum visible height for non-zero

    int y = chartTop + chartHeight - barHeight;

    return QRect(x, y, (int)barWidth, barHeight);
}

void StakingChartWidget::rebuildCache()
{
    int w = width();
    int h = height();
    if (w <= 0 || h <= 0) return;

    paintCache = QPixmap(w, h);
    paintCache.fill(Qt::transparent);

    QPainter painter(&paintCache);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int chartLeft = CHART_PADDING_LEFT;
    int chartTop = CHART_PADDING_TOP;
    int chartWidth = w - CHART_PADDING_LEFT - CHART_PADDING_RIGHT;
    int chartHeight = h - CHART_PADDING_TOP - CHART_PADDING_BOTTOM;

    if (chartWidth <= 0 || chartHeight <= 0)
        return;

    // Find max amount for scaling
    CAmount maxAmount = 0;
    for (int i = 0; i < chartData.size(); i++) {
        if (chartData[i].amount > maxAmount)
            maxAmount = chartData[i].amount;
    }

    // If no data or all zeros, show empty state
    if (chartData.isEmpty() || maxAmount == 0) {
        painter.setPen(COLOR_AXIS_TEXT);
        QFont emptyFont = painter.font();
        emptyFont.setPixelSize(12);
        painter.setFont(emptyFont);
        painter.drawText(QRect(0, 0, w, h), Qt::AlignCenter, "No staking rewards in the last 30 days");
        cacheDirty = false;
        cachedHoverBar = hoveredBar;
        return;
    }

    // Add 10% headroom
    maxAmount = (CAmount)(maxAmount * 1.1);

    // Draw horizontal grid lines (4 lines)
    painter.setPen(QPen(COLOR_GRID, 1));
    QFont axisFont = painter.font();
    axisFont.setPixelSize(10);
    painter.setFont(axisFont);

    for (int i = 0; i <= 4; i++) {
        int y = chartTop + (int)((double)i / 4.0 * chartHeight);
        painter.setPen(QPen(COLOR_GRID, 1, Qt::DotLine));
        painter.drawLine(chartLeft, y, chartLeft + chartWidth, y);

        // Y-axis label
        CAmount labelAmount = (CAmount)((double)(4 - i) / 4.0 * maxAmount);
        QString label = BitcoinUnits::format(displayUnit, labelAmount, false, BitcoinUnits::separatorAlways, 2);
        painter.setPen(COLOR_AXIS_TEXT);
        painter.drawText(0, y - 7, chartLeft - 5, 14, Qt::AlignRight | Qt::AlignVCenter, label);
    }

    // Draw bars
    int numBars = chartData.size();
    double barWidth = (double)(chartWidth - (numBars - 1) * BAR_SPACING) / numBars;
    if (barWidth < 1) barWidth = 1;

    for (int i = 0; i < numBars; i++) {
        QRect barRect = getBarRect(i, chartLeft, chartTop, chartWidth, chartHeight, maxAmount);

        QColor barColor;
        if (chartData[i].amount == 0) {
            barRect = QRect(barRect.x(), chartTop + chartHeight - 1, (int)barWidth, 1);
            barColor = COLOR_BAR_ZERO;
        } else if (i == hoveredBar) {
            barColor = COLOR_BAR_HOVER;
        } else {
            barColor = COLOR_BAR_NORMAL;
        }

        // Rounded top corners
        QPainterPath path;
        int radius = barWidth > 6 ? 3 : (barWidth > 3 ? 1 : 0);
        if (barRect.height() > radius * 2) {
            path.moveTo(barRect.bottomLeft());
            path.lineTo(barRect.left(), barRect.top() + radius);
            path.quadTo(barRect.topLeft(), QPointF(barRect.left() + radius, barRect.top()));
            path.lineTo(barRect.right() - radius, barRect.top());
            path.quadTo(barRect.topRight(), QPointF(barRect.right(), barRect.top() + radius));
            path.lineTo(barRect.bottomRight());
            path.closeSubpath();
        } else {
            path.addRect(barRect);
        }

        painter.fillPath(path, barColor);

        // X-axis date labels
        int labelEvery = numBars > 15 ? 5 : (numBars > 7 ? 3 : 1);
        if (i % labelEvery == 0 || i == numBars - 1) {
            painter.setPen(COLOR_AXIS_TEXT);
            int labelX = barRect.left();
            int labelW = (int)(barWidth * labelEvery);
            if (i == numBars - 1) labelW = (int)barWidth + CHART_PADDING_RIGHT;
            painter.drawText(labelX, chartTop + chartHeight + 4, labelW, 20,
                           Qt::AlignLeft | Qt::AlignTop, chartData[i].label);
        }
    }

    cacheDirty = false;
    cachedHoverBar = hoveredBar;
}

void StakingChartWidget::paintEvent(QPaintEvent *)
{
    // Only rebuild cache when data, size, or hover state changed
    if (cacheDirty || cachedHoverBar != hoveredBar || paintCache.size() != size()) {
        rebuildCache();
    }

    QPainter painter(this);
    if (!paintCache.isNull()) {
        painter.drawPixmap(0, 0, paintCache);
    }
}

void StakingChartWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (chartData.isEmpty()) return;

    CAmount maxAmount = 0;
    for (int i = 0; i < chartData.size(); i++) {
        if (chartData[i].amount > maxAmount)
            maxAmount = chartData[i].amount;
    }
    if (maxAmount == 0) return;

    int chartLeft = CHART_PADDING_LEFT;
    int chartWidth = width() - CHART_PADDING_LEFT - CHART_PADDING_RIGHT;

    int newHover = -1;
    int numBars = chartData.size();
    double barWidth = (double)(chartWidth - (numBars - 1) * BAR_SPACING) / numBars;

    int mouseX = event->pos().x();
    for (int i = 0; i < numBars; i++) {
        int x = chartLeft + (int)(i * (barWidth + BAR_SPACING));
        if (mouseX >= x && mouseX < x + (int)barWidth) {
            newHover = i;
            break;
        }
    }

    if (newHover != hoveredBar) {
        hoveredBar = newHover;
        update(); // will rebuild cache with new hover bar

        if (hoveredBar >= 0 && hoveredBar < chartData.size()) {
            QString tip = QString("%1\n%2")
                .arg(chartData[hoveredBar].label)
                .arg(BitcoinUnits::formatWithUnit(displayUnit, chartData[hoveredBar].amount, false, BitcoinUnits::separatorAlways, 2));
            QToolTip::showText(mapToGlobal(event->pos()), tip, this);
        }
    }
}

void StakingChartWidget::leaveEvent(QEvent *)
{
    if (hoveredBar != -1) {
        hoveredBar = -1;
        update();
    }
}
