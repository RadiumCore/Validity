// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2025-2026 The Validity developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "carddragdrop.h"

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QSettings>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QAbstractScrollArea>
#include <QScrollBar>
#include <QProgressBar>
#include <climits>

DashboardGridManager::DashboardGridManager(QWidget *parent)
    : QObject(parent),
      dashboard(parent),
      hSplitter(0), leftColumn(0), rightColumn(0),
      isDragging(false), dragCard(0),
      dragPreview(0), dropLine(0),
      targetCol(0), targetIdx(-1)
{
}

DashboardGridManager::~DashboardGridManager()
{
    saveLayout();
    if (isDragging) {
        qApp->removeEventFilter(this);
        isDragging = false;
        if (dragPreview) {
            delete dragPreview;
            dragPreview = 0;
        }
        if (dropLine) dropLine->hide();
    }
}

// ---------------------------------------------------------------------------
//  Setup
// ---------------------------------------------------------------------------

void DashboardGridManager::installFilters(QWidget *w)
{
    w->installEventFilter(this);
    Q_FOREACH(QObject *child, w->children()) {
        QWidget *cw = qobject_cast<QWidget*>(child);
        if (cw) installFilters(cw);
    }
}

QWidget* DashboardGridManager::cardForWidget(QWidget *w)
{
    while (w) {
        if (allCards.contains(w)) return w;
        w = w->parentWidget();
    }
    return 0;
}

bool DashboardGridManager::isInteractive(QWidget *w)
{
    // Walk from the event source up to the card; if any ancestor (before
    // the card itself) is an interactive control, suppress drag initiation.
    while (w && !allCards.contains(w)) {
        if (qobject_cast<QPushButton*>(w) ||
            qobject_cast<QLineEdit*>(w) ||
            qobject_cast<QComboBox*>(w) ||
            qobject_cast<QAbstractScrollArea*>(w) ||
            qobject_cast<QScrollBar*>(w) ||
            qobject_cast<QProgressBar*>(w))
            return true;

        // Viewports of scroll areas are plain QWidgets — catch them too
        QWidget *par = w->parentWidget();
        if (par) {
            QAbstractScrollArea *sa = qobject_cast<QAbstractScrollArea*>(par);
            if (sa && sa->viewport() == w)
                return true;
        }
        w = w->parentWidget();
    }
    return false;
}

QSplitter* DashboardGridManager::setupGrid(QList<QWidget*> cards)
{
    allCards = cards;

    leftColumn = new QSplitter(Qt::Vertical, dashboard);
    leftColumn->setChildrenCollapsible(false);
    leftColumn->setMinimumWidth(180);

    rightColumn = new QSplitter(Qt::Vertical, dashboard);
    rightColumn->setChildrenCollapsible(false);
    rightColumn->setMinimumWidth(180);

    hSplitter = new QSplitter(Qt::Horizontal, dashboard);
    hSplitter->setChildrenCollapsible(false);
    hSplitter->addWidget(leftColumn);
    hSplitter->addWidget(rightColumn);
    hSplitter->setStretchFactor(0, 1);
    hSplitter->setStretchFactor(1, 1);

    // Try to restore a saved arrangement; fall back to defaults.
    if (!restoreFromSettings()) {
        // Default: first 3 cards left, last card (transactions) right
        for (int i = 0; i < cards.size(); i++) {
            if (i < cards.size() - 1)
                leftColumn->addWidget(cards[i]);
            else
                rightColumn->addWidget(cards[i]);
        }
    }

    // Install event filters on every widget inside every card
    Q_FOREACH(QWidget *card, cards) {
        installFilters(card);
    }

    // Green drop indicator (hidden until a drag is active)
    dropLine = new QWidget(dashboard);
    dropLine->setFixedHeight(4);
    dropLine->setStyleSheet("background-color: #43b581; border-radius: 2px;");
    dropLine->hide();

    // Persist splitter sizes when the user resizes
    connect(hSplitter, SIGNAL(splitterMoved(int,int)), this, SLOT(saveLayout()));
    connect(leftColumn, SIGNAL(splitterMoved(int,int)), this, SLOT(saveLayout()));
    connect(rightColumn, SIGNAL(splitterMoved(int,int)), this, SLOT(saveLayout()));

    return hSplitter;
}

// ---------------------------------------------------------------------------
//  Event filter
// ---------------------------------------------------------------------------

bool DashboardGridManager::eventFilter(QObject *obj, QEvent *event)
{
    // ---- Global capture while dragging (installed on qApp) ----
    if (isDragging) {
        switch (event->type()) {
        case QEvent::MouseMove: {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            moveDrag(me->globalPos());
            return true;
        }
        case QEvent::MouseButtonRelease:
            endDrag();
            return true;
        case QEvent::KeyPress: {
            QKeyEvent *ke = static_cast<QKeyEvent*>(event);
            if (ke->key() == Qt::Key_Escape) {
                abortDrag();
                return true;
            }
            break;
        }
        default:
            break;
        }
        return false;
    }

    // ---- Normal mode: detect drag start on card children ----
    QWidget *w = qobject_cast<QWidget*>(obj);
    if (!w) return false;

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton && !isInteractive(w)) {
            QWidget *card = cardForWidget(w);
            if (card) {
                dragCard = card;
                dragStartPos = me->globalPos();
            }
        }
        break;
    }
    case QEvent::MouseMove: {
        if (dragCard) {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            if ((me->globalPos() - dragStartPos).manhattanLength() >= DRAG_DIST) {
                beginDrag(dragCard, me->globalPos());
                return true;
            }
        }
        break;
    }
    case QEvent::MouseButtonRelease:
        dragCard = 0;
        break;
    default:
        break;
    }

    return false;
}

// ---------------------------------------------------------------------------
//  Drag lifecycle
// ---------------------------------------------------------------------------

void DashboardGridManager::beginDrag(QWidget *card, const QPoint &globalPos)
{
    isDragging = true;

    // Capture all mouse events globally
    qApp->installEventFilter(this);

    // Floating thumbnail preview
    QPixmap pix = card->grab();
    dragPreview = new QLabel(0);
    dragPreview->setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    dragPreview->setAttribute(Qt::WA_TransparentForMouseEvents);
    dragPreview->setPixmap(pix.scaled(pix.width() * 6 / 10,
                                       pix.height() * 6 / 10,
                                       Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation));
    dragPreview->setWindowOpacity(0.65);
    dragPreview->adjustSize();
    dragPreview->move(globalPos - QPoint(dragPreview->width() / 2, 20));
    dragPreview->show();

    moveDrag(globalPos);
}

void DashboardGridManager::moveDrag(const QPoint &globalPos)
{
    if (dragPreview)
        dragPreview->move(globalPos - QPoint(dragPreview->width() / 2, 20));

    QPair<QSplitter*, int> target = findTarget(globalPos);
    targetCol = target.first;
    targetIdx = target.second;

    if (targetCol)
        positionDropLine(targetCol, targetIdx);
    else
        hideDropLine();
}

void DashboardGridManager::endDrag()
{
    qApp->removeEventFilter(this);
    isDragging = false;

    if (dragPreview) {
        dragPreview->hide();
        dragPreview->deleteLater();
        dragPreview = 0;
    }
    hideDropLine();

    if (dragCard && targetCol && targetIdx >= 0) {
        QSplitter *fromCol = qobject_cast<QSplitter*>(dragCard->parentWidget());
        if (fromCol) {
            int fromIdx = fromCol->indexOf(dragCard);
            int adj = targetIdx;
            if (fromCol == targetCol && fromIdx < adj)
                adj--;

            if (fromCol != targetCol || fromIdx != adj) {
                targetCol->insertWidget(adj, dragCard);
                saveLayout();
            }
        }
    }

    dragCard = 0;
    targetCol = 0;
    targetIdx = -1;
}

void DashboardGridManager::abortDrag()
{
    qApp->removeEventFilter(this);
    isDragging = false;

    if (dragPreview) {
        dragPreview->hide();
        dragPreview->deleteLater();
        dragPreview = 0;
    }
    hideDropLine();

    dragCard = 0;
    targetCol = 0;
    targetIdx = -1;
}

// ---------------------------------------------------------------------------
//  Hit testing & visual feedback
// ---------------------------------------------------------------------------

QPair<QSplitter*, int> DashboardGridManager::findTarget(const QPoint &globalPos)
{
    if (!leftColumn || !rightColumn)
        return qMakePair((QSplitter*)0, -1);

    // Determine which column the cursor is over
    QRect leftGeo = QRect(leftColumn->mapToGlobal(QPoint(0, 0)), leftColumn->size());
    QRect rightGeo = QRect(rightColumn->mapToGlobal(QPoint(0, 0)), rightColumn->size());

    QSplitter *col = 0;
    if (leftGeo.contains(globalPos))
        col = leftColumn;
    else if (rightGeo.contains(globalPos))
        col = rightColumn;
    else {
        int dL = qAbs(globalPos.x() - leftGeo.center().x());
        int dR = qAbs(globalPos.x() - rightGeo.center().x());
        col = (dL <= dR) ? leftColumn : rightColumn;
    }

    // Find the insertion index closest to the cursor Y
    int count = col->count();
    if (count == 0)
        return qMakePair(col, 0);

    int bestIdx = count;
    int bestDist = INT_MAX;

    for (int i = 0; i <= count; i++) {
        int y;
        if (i == 0) {
            y = col->widget(0)->mapToGlobal(QPoint(0, 0)).y();
        } else {
            QWidget *prev = col->widget(i - 1);
            y = prev->mapToGlobal(QPoint(0, prev->height())).y();
        }

        int dist = qAbs(globalPos.y() - y);
        if (dist < bestDist) {
            bestDist = dist;
            bestIdx = i;
        }
    }

    return qMakePair(col, bestIdx);
}

void DashboardGridManager::positionDropLine(QSplitter *col, int idx)
{
    if (!dropLine || !col) return;

    int y;
    if (col->count() == 0) {
        y = col->mapTo(dashboard, QPoint(0, col->height() / 2)).y();
    } else if (idx <= 0) {
        QWidget *first = col->widget(0);
        y = first->mapTo(dashboard, QPoint(0, 0)).y();
    } else if (idx >= col->count()) {
        QWidget *last = col->widget(col->count() - 1);
        y = last->mapTo(dashboard, QPoint(0, last->height())).y();
    } else {
        QWidget *above = col->widget(idx - 1);
        y = above->mapTo(dashboard, QPoint(0, above->height())).y();
    }

    QPoint colOrigin = col->mapTo(dashboard, QPoint(0, 0));
    dropLine->setGeometry(colOrigin.x() + 4, y - 2, col->width() - 8, 4);
    dropLine->raise();
    dropLine->show();
}

void DashboardGridManager::hideDropLine()
{
    if (dropLine) dropLine->hide();
}

// ---------------------------------------------------------------------------
//  Persistence
// ---------------------------------------------------------------------------

void DashboardGridManager::saveLayout()
{
    if (!leftColumn || !rightColumn || !hSplitter) return;

    QSettings settings;
    QStringList leftNames, rightNames;

    for (int i = 0; i < leftColumn->count(); i++)
        leftNames << leftColumn->widget(i)->objectName();
    for (int i = 0; i < rightColumn->count(); i++)
        rightNames << rightColumn->widget(i)->objectName();

    settings.setValue("DashGridLeft", leftNames);
    settings.setValue("DashGridRight", rightNames);
    settings.setValue("DashGridH", hSplitter->saveState());
    settings.setValue("DashGridLV", leftColumn->saveState());
    settings.setValue("DashGridRV", rightColumn->saveState());
}

bool DashboardGridManager::restoreFromSettings()
{
    QSettings settings;
    QStringList leftNames = settings.value("DashGridLeft").toStringList();
    QStringList rightNames = settings.value("DashGridRight").toStringList();

    if (leftNames.isEmpty() && rightNames.isEmpty())
        return false;

    // Build name → widget map
    QMap<QString, QWidget*> cardMap;
    Q_FOREACH(QWidget *card, allCards)
        cardMap[card->objectName()] = card;

    Q_FOREACH(const QString &name, leftNames) {
        if (cardMap.contains(name))
            leftColumn->addWidget(cardMap.take(name));
    }
    Q_FOREACH(const QString &name, rightNames) {
        if (cardMap.contains(name))
            rightColumn->addWidget(cardMap.take(name));
    }

    // Any cards not in the saved layout go to left column
    Q_FOREACH(QWidget *card, cardMap.values())
        leftColumn->addWidget(card);

    // Restore splitter geometry
    QByteArray hState = settings.value("DashGridH").toByteArray();
    QByteArray lvState = settings.value("DashGridLV").toByteArray();
    QByteArray rvState = settings.value("DashGridRV").toByteArray();

    if (!hState.isEmpty()) hSplitter->restoreState(hState);
    if (!lvState.isEmpty()) leftColumn->restoreState(lvState);
    if (!rvState.isEmpty()) rightColumn->restoreState(rvState);

    return true;
}
