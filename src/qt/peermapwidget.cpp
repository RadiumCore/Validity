// Copyright (c) 2026 The Validity developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "peermapwidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QPainterPath>
#include <QtMath>

PeerMapWidget::PeerMapWidget(QWidget *parent)
    : QWidget(parent), hoveredPeer(-1)
{
    setMouseTracking(true);
    setMinimumSize(400, 250);
}

void PeerMapWidget::setPeers(const QVector<PeerMapNode> &peers)
{
    peerNodes = peers;
    hoveredPeer = -1;
    update();
}

void PeerMapWidget::clear()
{
    peerNodes.clear();
    hoveredPeer = -1;
    update();
}

QSize PeerMapWidget::minimumSizeHint() const { return QSize(400, 250); }
QSize PeerMapWidget::sizeHint() const { return QSize(600, 350); }

QPointF PeerMapWidget::geoToScreen(double lat, double lon, const QRectF &mapRect) const
{
    // Simple equirectangular projection
    double x = (lon + 180.0) / 360.0 * mapRect.width() + mapRect.left();
    double y = (90.0 - lat) / 180.0 * mapRect.height() + mapRect.top();
    return QPointF(x, y);
}

QColor PeerMapWidget::pingColor(int pingMs) const
{
    if (pingMs < 0) return QColor("#5a5a7a");       // unknown
    if (pingMs < 100) return QColor("#43b581");      // green - good
    if (pingMs < 300) return QColor("#faa61a");      // yellow - ok
    return QColor("#e05555");                         // red - poor
}

void PeerMapWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Background
    painter.fillRect(rect(), QColor("#0a0a1a"));

    // Map area with margin
    QRectF mapRect = QRectF(rect()).adjusted(20, 20, -20, -40);

    // Draw grid lines (subtle)
    painter.setPen(QPen(QColor(30, 30, 60), 0.5));
    for (int lon = -180; lon <= 180; lon += 60) {
        QPointF top = geoToScreen(85, lon, mapRect);
        QPointF bot = geoToScreen(-85, lon, mapRect);
        painter.drawLine(top, bot);
    }
    for (int lat = -60; lat <= 80; lat += 30) {
        QPointF left = geoToScreen(lat, -180, mapRect);
        QPointF right = geoToScreen(lat, 180, mapRect);
        painter.drawLine(left, right);
    }

    // Draw continent outlines
    drawWorldOutline(painter, mapRect);

    // Our position (center of map)
    QPointF center = mapRect.center();

    // Draw connection lines first (below dots)
    for (int i = 0; i < peerNodes.size(); i++) {
        const PeerMapNode &peer = peerNodes[i];
        QPointF pos = geoToScreen(peer.latitude, peer.longitude, mapRect);

        QColor lineColor = pingColor(peer.pingMs);
        lineColor.setAlpha(40);
        painter.setPen(QPen(lineColor, 1.0, Qt::DashLine));
        painter.drawLine(center, pos);
    }

    // Draw center node (us)
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#43b581"));
    painter.drawEllipse(center, 6, 6);
    painter.setPen(QPen(QColor("#43b581"), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(center, 10, 10);

    // Draw peer dots
    for (int i = 0; i < peerNodes.size(); i++) {
        const PeerMapNode &peer = peerNodes[i];
        QPointF pos = geoToScreen(peer.latitude, peer.longitude, mapRect);
        drawPeerDot(painter, peer, pos, i == hoveredPeer);
    }

    // Draw tooltip for hovered peer
    if (hoveredPeer >= 0 && hoveredPeer < peerNodes.size()) {
        QPointF pos = geoToScreen(peerNodes[hoveredPeer].latitude,
                                   peerNodes[hoveredPeer].longitude, mapRect);
        drawTooltip(painter, peerNodes[hoveredPeer], pos);
    }

    // Legend
    painter.setFont(QFont(painter.font().family(), 9));
    int ly = rect().bottom() - 18;
    int lx = 25;

    painter.setBrush(QColor("#43b581"));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(lx, ly), 4, 4);
    painter.setPen(QColor("#8888a8"));
    painter.drawText(lx + 8, ly + 4, "<100ms");

    lx += 70;
    painter.setBrush(QColor("#faa61a"));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(lx, ly), 4, 4);
    painter.setPen(QColor("#8888a8"));
    painter.drawText(lx + 8, ly + 4, "<300ms");

    lx += 70;
    painter.setBrush(QColor("#e05555"));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(lx, ly), 4, 4);
    painter.setPen(QColor("#8888a8"));
    painter.drawText(lx + 8, ly + 4, ">300ms");

    // Peer count
    painter.setPen(QColor("#e8e8f0"));
    painter.drawText(rect().right() - 150, ly + 4,
        QString("%1 peers connected").arg(peerNodes.size()));
}

void PeerMapWidget::drawPeerDot(QPainter &painter, const PeerMapNode &peer,
                                 const QPointF &pos, bool hovered) const
{
    QColor color = pingColor(peer.pingMs);
    double radius = hovered ? 7 : 5;

    // Glow effect for hovered
    if (hovered) {
        QColor glow = color;
        glow.setAlpha(60);
        painter.setPen(Qt::NoPen);
        painter.setBrush(glow);
        painter.drawEllipse(pos, 12, 12);
    }

    // Dot
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(pos, radius, radius);

    // Inbound indicator (small ring)
    if (peer.isInbound) {
        painter.setPen(QPen(QColor("#ffffff"), 1.5));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(pos, radius + 2, radius + 2);
    }
}

void PeerMapWidget::drawTooltip(QPainter &painter, const PeerMapNode &peer,
                                 const QPointF &pos) const
{
    QString text = QString("%1\n%2\nPing: %3ms\nIn: %4 KB / Out: %5 KB%6")
        .arg(peer.address)
        .arg(peer.subversion)
        .arg(peer.pingMs >= 0 ? QString::number(peer.pingMs) : "?")
        .arg(peer.bytesIn / 1024)
        .arg(peer.bytesOut / 1024)
        .arg(peer.isInbound ? "\n(inbound)" : "");

    QFont font = painter.font();
    font.setPointSize(10);
    QFontMetrics fm(font);
    QStringList lines = text.split('\n');

    int maxWidth = 0;
    for (int i = 0; i < lines.size(); i++) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        maxWidth = qMax(maxWidth, fm.horizontalAdvance(lines[i]));
#else
        maxWidth = qMax(maxWidth, fm.width(lines[i]));
#endif
    }

    int padding = 8;
    int lineHeight = fm.height();
    QRectF bgRect(pos.x() + 12, pos.y() - 10,
                  maxWidth + padding * 2, lines.size() * lineHeight + padding * 2);

    // Keep on screen
    if (bgRect.right() > rect().right() - 10)
        bgRect.moveRight(pos.x() - 12);
    if (bgRect.bottom() > rect().bottom() - 30)
        bgRect.moveBottom(pos.y() - 10);

    painter.setPen(QPen(QColor("#2a2a4d"), 1));
    painter.setBrush(QColor(15, 15, 36, 240));
    painter.drawRoundedRect(bgRect, 6, 6);

    painter.setPen(QColor("#e8e8f0"));
    painter.setFont(font);
    for (int i = 0; i < lines.size(); i++) {
        painter.drawText(bgRect.left() + padding,
                         bgRect.top() + padding + (i + 1) * lineHeight - fm.descent(),
                         lines[i]);
    }
}

void PeerMapWidget::mouseMoveEvent(QMouseEvent *event)
{
    QRectF mapRect = QRectF(rect()).adjusted(20, 20, -20, -40);
    int closest = -1;
    double minDist = 15.0; // pixel threshold

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPointF mousePos = event->position();
#else
    QPointF mousePos = event->pos();
#endif

    for (int i = 0; i < peerNodes.size(); i++) {
        QPointF pos = geoToScreen(peerNodes[i].latitude, peerNodes[i].longitude, mapRect);
        double dx = mousePos.x() - pos.x();
        double dy = mousePos.y() - pos.y();
        double dist = qSqrt(dx * dx + dy * dy);
        if (dist < minDist) {
            minDist = dist;
            closest = i;
        }
    }

    if (closest != hoveredPeer) {
        hoveredPeer = closest;
        update();
    }
}

void PeerMapWidget::leaveEvent(QEvent *)
{
    if (hoveredPeer >= 0) {
        hoveredPeer = -1;
        update();
    }
}

void PeerMapWidget::drawWorldOutline(QPainter &painter, const QRectF &mapRect) const
{
    painter.setPen(QPen(QColor(40, 40, 80), 1.2));
    painter.setBrush(QColor(20, 20, 45));

    // Simplified continent outlines (lat, lon pairs)
    // North America
    {
        QPainterPath path;
        double pts[][2] = {
            {50,-130},{55,-125},{60,-140},{65,-170},{72,-155},{72,-95},{60,-65},
            {47,-55},{43,-65},{40,-74},{30,-82},{25,-80},{25,-97},{30,-105},
            {32,-117},{38,-122},{48,-124},{50,-130}
        };
        path.moveTo(geoToScreen(pts[0][0], pts[0][1], mapRect));
        for (int i = 1; i < 18; i++)
            path.lineTo(geoToScreen(pts[i][0], pts[i][1], mapRect));
        path.closeSubpath();
        painter.drawPath(path);
    }

    // South America
    {
        QPainterPath path;
        double pts[][2] = {
            {12,-72},{7,-77},{0,-80},{-5,-81},{-15,-76},{-22,-70},{-35,-57},
            {-52,-68},{-55,-70},{-55,-65},{-40,-62},{-35,-55},
            {-22,-40},{-10,-37},{-3,-50},{5,-60},{10,-72},{12,-72}
        };
        path.moveTo(geoToScreen(pts[0][0], pts[0][1], mapRect));
        for (int i = 1; i < 18; i++)
            path.lineTo(geoToScreen(pts[i][0], pts[i][1], mapRect));
        path.closeSubpath();
        painter.drawPath(path);
    }

    // Europe
    {
        QPainterPath path;
        double pts[][2] = {
            {36,-9},{37,0},{43,5},{46,14},{42,18},{40,20},{38,24},
            {41,29},{45,30},{50,30},{55,20},{57,24},{60,25},{68,28},
            {71,25},{70,20},{64,14},{58,12},{55,8},{54,10},{53,5},
            {51,4},{49,-1},{48,-5},{43,-9},{36,-9}
        };
        path.moveTo(geoToScreen(pts[0][0], pts[0][1], mapRect));
        for (int i = 1; i < 26; i++)
            path.lineTo(geoToScreen(pts[i][0], pts[i][1], mapRect));
        path.closeSubpath();
        painter.drawPath(path);
    }

    // Africa
    {
        QPainterPath path;
        double pts[][2] = {
            {37,10},{32,13},{30,32},{22,37},{12,44},{2,42},{-11,40},
            {-15,42},{-26,33},{-34,18},{-30,17},{-17,12},{-5,12},
            {5,1},{5,-5},{7,-8},{5,-2},{6,2},{4,10},{0,10},
            {5,-10},{15,-17},{22,-17},{30,-10},{35,-1},{37,10}
        };
        path.moveTo(geoToScreen(pts[0][0], pts[0][1], mapRect));
        for (int i = 1; i < 26; i++)
            path.lineTo(geoToScreen(pts[i][0], pts[i][1], mapRect));
        path.closeSubpath();
        painter.drawPath(path);
    }

    // Asia
    {
        QPainterPath path;
        double pts[][2] = {
            {42,32},{40,44},{38,58},{25,57},{23,68},{28,77},{22,88},
            {21,92},{28,97},{23,104},{22,114},{30,122},{35,129},
            {40,132},{45,142},{50,143},{55,137},{60,130},{65,140},
            {68,170},{72,180},{72,120},{70,90},{68,60},{60,60},
            {55,70},{50,55},{45,40},{42,32}
        };
        path.moveTo(geoToScreen(pts[0][0], pts[0][1], mapRect));
        for (int i = 1; i < 28; i++)
            path.lineTo(geoToScreen(pts[i][0], pts[i][1], mapRect));
        path.closeSubpath();
        painter.drawPath(path);
    }

    // Australia
    {
        QPainterPath path;
        double pts[][2] = {
            {-12,132},{-14,127},{-22,114},{-32,115},{-35,117},{-35,138},
            {-38,146},{-37,150},{-33,152},{-28,153},{-24,150},{-18,146},
            {-14,144},{-12,142},{-11,136},{-12,132}
        };
        path.moveTo(geoToScreen(pts[0][0], pts[0][1], mapRect));
        for (int i = 1; i < 16; i++)
            path.lineTo(geoToScreen(pts[i][0], pts[i][1], mapRect));
        path.closeSubpath();
        painter.drawPath(path);
    }
}
