// Copyright (c) 2026 The Validity developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_CARDDRAGDROP_H
#define BITCOIN_QT_CARDDRAGDROP_H

#include <QObject>
#include <QPoint>
#include <QStringList>
#include <QMap>

class QVBoxLayout;
class QHBoxLayout;
class QWidget;

/**
 * Manages drag-and-drop reordering and vertical resizing of card widgets
 * within a layout. Installed as an event filter on each card frame.
 *
 * Drag: click and drag a card to reorder it among other cards.
 * Resize: hover near the bottom edge of a card to get a resize cursor,
 *         then drag to change the card's minimum height.
 */
class CardDragDropManager : public QObject
{
    Q_OBJECT

public:
    explicit CardDragDropManager(QWidget *container, QObject *parent = 0);

    /** Register a card widget for drag-drop reordering and resizing */
    void registerCard(QWidget *card);

    /** Save current card order and sizes to QSettings */
    void saveOrder();

    /** Restore saved card order and sizes from QSettings */
    void restoreOrder();

protected:
    bool eventFilter(QObject *obj, QEvent *event) Q_DECL_OVERRIDE;

private:
    void startDrag(QWidget *card, const QPoint &pos);
    void updateDropIndicator(const QPoint &globalPos);
    void finishDrop(const QPoint &globalPos);
    void cancelDrag();
    int findInsertIndex(const QPoint &globalPos);

    bool isNearBottomEdge(QWidget *card, const QPoint &localPos) const;
    void startResize(QWidget *card, const QPoint &globalPos);
    void updateResize(const QPoint &globalPos);
    void finishResize();

    QWidget *container;
    QList<QWidget*> cards;

    // Drag state
    bool dragging;
    QWidget *dragSource;
    QPoint dragStartPos;
    QWidget *dropIndicator;

    // Resize state
    bool resizing;
    QWidget *resizeSource;
    QPoint resizeStartPos;
    int resizeStartHeight;

    static const int RESIZE_EDGE_MARGIN = 8;
    static const int MIN_CARD_HEIGHT = 60;
};

#endif // BITCOIN_QT_CARDDRAGDROP_H
