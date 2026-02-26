// Copyright (c) 2026 The Validity developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_CARDDRAGDROP_H
#define BITCOIN_QT_CARDDRAGDROP_H

#include <QObject>
#include <QPoint>
#include <QStringList>

class QVBoxLayout;
class QHBoxLayout;
class QWidget;

/**
 * Manages drag-and-drop reordering of card widgets within a layout.
 * Installed as an event filter on each card frame.
 */
class CardDragDropManager : public QObject
{
    Q_OBJECT

public:
    explicit CardDragDropManager(QWidget *container, QObject *parent = 0);

    /** Register a card widget for drag-drop reordering */
    void registerCard(QWidget *card);

    /** Save current card order to QSettings */
    void saveOrder();

    /** Restore saved card order from QSettings */
    void restoreOrder();

protected:
    bool eventFilter(QObject *obj, QEvent *event) Q_DECL_OVERRIDE;

private:
    void startDrag(QWidget *card, const QPoint &pos);
    void updateDropIndicator(const QPoint &globalPos);
    void finishDrop(const QPoint &globalPos);
    void cancelDrag();
    int findInsertIndex(const QPoint &globalPos);

    QWidget *container;
    QList<QWidget*> cards;

    // Drag state
    bool dragging;
    QWidget *dragSource;
    QPoint dragStartPos;
    QWidget *dropIndicator;
};

#endif // BITCOIN_QT_CARDDRAGDROP_H
