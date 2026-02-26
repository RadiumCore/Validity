// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2025-2026 The Validity developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_CARDDRAGDROP_H
#define BITCOIN_QT_CARDDRAGDROP_H

#include <QObject>
#include <QWidget>
#include <QSplitter>
#include <QPoint>
#include <QList>

class QLabel;

/**
 * Manages a 2-column resizable dashboard grid with drag-and-drop card
 * rearrangement.  Cards can be dragged between columns and reordered
 * within a column.  Both column widths and individual card heights are
 * user-resizable via QSplitter handles.  Layout is persisted to QSettings.
 */
class DashboardGridManager : public QObject
{
    Q_OBJECT

public:
    explicit DashboardGridManager(QWidget *parent = 0);
    ~DashboardGridManager();

    /** Build the grid and return the top-level splitter to add to a layout. */
    QSplitter* setupGrid(QList<QWidget*> cards);

public Q_SLOTS:
    /** Persist current card arrangement and splitter sizes to QSettings. */
    void saveLayout();

protected:
    bool eventFilter(QObject *obj, QEvent *event) Q_DECL_OVERRIDE;

private:
    QWidget *dashboard;
    QSplitter *hSplitter;       // horizontal: left column | right column
    QSplitter *leftColumn;      // vertical splitter for left cards
    QSplitter *rightColumn;     // vertical splitter for right cards
    QList<QWidget*> allCards;

    // Drag state
    bool isDragging;
    QWidget *dragCard;
    QPoint dragStartPos;
    QLabel *dragPreview;        // floating thumbnail following the cursor
    QWidget *dropLine;          // green bar showing where the card will land

    // Current drop target
    QSplitter *targetCol;
    int targetIdx;

    static const int DRAG_DIST = 20;

    // Helpers
    QWidget* cardForWidget(QWidget *w);
    bool isInteractive(QWidget *w);
    void installFilters(QWidget *w);

    // Drag lifecycle
    void beginDrag(QWidget *card, const QPoint &globalPos);
    void moveDrag(const QPoint &globalPos);
    void endDrag();
    void abortDrag();

    // Hit testing and visual feedback
    QPair<QSplitter*, int> findTarget(const QPoint &globalPos);
    void positionDropLine(QSplitter *col, int idx);
    void hideDropLine();

    // Persistence
    bool restoreFromSettings();
};

#endif // BITCOIN_QT_CARDDRAGDROP_H
