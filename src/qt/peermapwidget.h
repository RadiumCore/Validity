// Copyright (c) 2026 The Validity developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PEERMAPWIDGET_H
#define BITCOIN_QT_PEERMAPWIDGET_H

#include <QWidget>
#include <QVector>
#include <QPair>

QT_BEGIN_NAMESPACE
class QPaintEvent;
class QMouseEvent;
QT_END_NAMESPACE

struct PeerMapNode {
    QString address;
    QString subversion;
    double latitude;
    double longitude;
    int pingMs;
    qint64 bytesIn;
    qint64 bytesOut;
    bool isInbound;
};

class PeerMapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PeerMapWidget(QWidget *parent = 0);

    void setPeers(const QVector<PeerMapNode> &peers);
    void clear();

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QVector<PeerMapNode> peerNodes;
    int hoveredPeer;

    QPointF geoToScreen(double lat, double lon, const QRectF &mapRect) const;
    void drawWorldOutline(QPainter &painter, const QRectF &mapRect) const;
    void drawPeerDot(QPainter &painter, const PeerMapNode &peer, const QPointF &pos, bool hovered) const;
    void drawTooltip(QPainter &painter, const PeerMapNode &peer, const QPointF &pos) const;
    QColor pingColor(int pingMs) const;
};

#endif // BITCOIN_QT_PEERMAPWIDGET_H
