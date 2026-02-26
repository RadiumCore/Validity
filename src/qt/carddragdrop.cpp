// Copyright (c) 2026 The Validity developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "carddragdrop.h"

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QLabel>

static const int DRAG_THRESHOLD = 20;
static const char* SETTINGS_KEY = "OverviewCardOrder";

CardDragDropManager::CardDragDropManager(QWidget *container, QObject *parent)
    : QObject(parent), container(container), dragging(false), dragSource(0), dropIndicator(0)
{
}

void CardDragDropManager::registerCard(QWidget *card)
{
    if (!card || cards.contains(card))
        return;
    cards.append(card);
    card->installEventFilter(this);
    card->setCursor(Qt::OpenHandCursor);
}

bool CardDragDropManager::eventFilter(QObject *obj, QEvent *event)
{
    QWidget *card = qobject_cast<QWidget*>(obj);
    if (!card || !cards.contains(card))
        return QObject::eventFilter(obj, event);

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            dragStartPos = me->globalPos();
            dragSource = card;
        }
        break;
    }
    case QEvent::MouseMove: {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        if (dragSource == card && !dragging && (me->buttons() & Qt::LeftButton)) {
            if ((me->globalPos() - dragStartPos).manhattanLength() >= DRAG_THRESHOLD) {
                startDrag(card, me->globalPos());
            }
        }
        if (dragging && dragSource == card) {
            updateDropIndicator(me->globalPos());
            return true;
        }
        break;
    }
    case QEvent::MouseButtonRelease: {
        if (dragging && dragSource == card) {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            finishDrop(me->globalPos());
            return true;
        }
        dragSource = 0;
        break;
    }
    default:
        break;
    }

    return QObject::eventFilter(obj, event);
}

void CardDragDropManager::startDrag(QWidget *card, const QPoint &pos)
{
    Q_UNUSED(pos);
    dragging = true;
    card->setCursor(Qt::ClosedHandCursor);

    // Create drop indicator line
    if (!dropIndicator) {
        dropIndicator = new QWidget(container);
        dropIndicator->setFixedHeight(3);
        dropIndicator->setStyleSheet("background-color: #43b581; border-radius: 1px;");
    }
    dropIndicator->hide();

    // Dim the dragged card slightly
    card->setStyleSheet(card->styleSheet() + "\n* { opacity: 0.6; }");
}

void CardDragDropManager::updateDropIndicator(const QPoint &globalPos)
{
    if (!dropIndicator || !container)
        return;

    int insertIdx = findInsertIndex(globalPos);
    if (insertIdx < 0) {
        dropIndicator->hide();
        return;
    }

    // Position the indicator between cards
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(container->layout());
    if (!layout)
        return;

    // Find the y position for the indicator
    int y = 0;
    if (insertIdx == 0) {
        // Before the first visible card
        for (int i = 0; i < layout->count(); i++) {
            QWidget *w = layout->itemAt(i)->widget();
            if (w && cards.contains(w)) {
                y = w->geometry().top() - 2;
                break;
            }
        }
    } else {
        // After the Nth visible card
        int cardsSeen = 0;
        for (int i = 0; i < layout->count(); i++) {
            QWidget *w = layout->itemAt(i)->widget();
            if (w && cards.contains(w)) {
                cardsSeen++;
                if (cardsSeen == insertIdx) {
                    y = w->geometry().bottom() + 2;
                    break;
                }
            }
        }
    }

    dropIndicator->setGeometry(16, y, container->width() - 32, 3);
    dropIndicator->show();
    dropIndicator->raise();
}

int CardDragDropManager::findInsertIndex(const QPoint &globalPos)
{
    QPoint localPos = container->mapFromGlobal(globalPos);
    int y = localPos.y();

    // Find which position the cursor is closest to
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(container->layout());
    if (!layout)
        return -1;

    int idx = 0;
    for (int i = 0; i < layout->count(); i++) {
        QWidget *w = layout->itemAt(i)->widget();
        if (!w || !cards.contains(w))
            continue;

        int cardMid = w->geometry().center().y();
        if (y > cardMid)
            idx++;
        else
            break;
    }
    return idx;
}

void CardDragDropManager::finishDrop(const QPoint &globalPos)
{
    if (!dragging || !dragSource)
        return;

    // Reset cursor and styling
    dragSource->setCursor(Qt::OpenHandCursor);
    dragSource->setStyleSheet("");

    if (dropIndicator)
        dropIndicator->hide();

    int targetIdx = findInsertIndex(globalPos);
    if (targetIdx < 0) {
        cancelDrag();
        return;
    }

    // Find the current index of the dragged card among our registered cards
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(container->layout());
    if (!layout) {
        cancelDrag();
        return;
    }

    // Get current card order (only our registered cards)
    QList<int> cardLayoutIndices;
    for (int i = 0; i < layout->count(); i++) {
        QWidget *w = layout->itemAt(i)->widget();
        if (w && cards.contains(w))
            cardLayoutIndices.append(i);
    }

    // Find source position in card-only ordering
    int sourceCardIdx = -1;
    int sourceLayoutIdx = -1;
    for (int i = 0; i < cardLayoutIndices.size(); i++) {
        QWidget *w = layout->itemAt(cardLayoutIndices[i])->widget();
        if (w == dragSource) {
            sourceCardIdx = i;
            sourceLayoutIdx = cardLayoutIndices[i];
            break;
        }
    }

    if (sourceCardIdx < 0 || sourceCardIdx == targetIdx) {
        cancelDrag();
        return;
    }

    // Remove from layout and reinsert
    QLayoutItem *item = layout->takeAt(sourceLayoutIdx);
    if (!item) {
        cancelDrag();
        return;
    }

    // Calculate new layout index based on target card position
    // After takeAt, indices shift, so recalculate
    int newLayoutIdx = 0;
    if (targetIdx == 0) {
        // Insert before first card
        for (int i = 0; i < layout->count(); i++) {
            QWidget *w = layout->itemAt(i)->widget();
            if (w && cards.contains(w)) {
                newLayoutIdx = i;
                break;
            }
        }
    } else {
        // Insert after the (targetIdx-1)th card
        int cardsSeen = 0;
        for (int i = 0; i < layout->count(); i++) {
            QWidget *w = layout->itemAt(i)->widget();
            if (w && cards.contains(w)) {
                cardsSeen++;
                if (cardsSeen == targetIdx) {
                    newLayoutIdx = i + 1;
                    break;
                }
            }
        }
    }

    layout->insertItem(newLayoutIdx, item);

    dragging = false;
    dragSource = 0;

    saveOrder();
}

void CardDragDropManager::cancelDrag()
{
    if (dragSource) {
        dragSource->setCursor(Qt::OpenHandCursor);
        dragSource->setStyleSheet("");
    }
    if (dropIndicator)
        dropIndicator->hide();
    dragging = false;
    dragSource = 0;
}

void CardDragDropManager::saveOrder()
{
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(container->layout());
    if (!layout)
        return;

    QStringList order;
    for (int i = 0; i < layout->count(); i++) {
        QWidget *w = layout->itemAt(i)->widget();
        if (w && cards.contains(w) && !w->objectName().isEmpty())
            order.append(w->objectName());
    }

    if (!order.isEmpty()) {
        QSettings settings;
        settings.setValue(SETTINGS_KEY, order);
    }
}

void CardDragDropManager::restoreOrder()
{
    QSettings settings;
    QStringList order = settings.value(SETTINGS_KEY).toStringList();
    if (order.isEmpty())
        return;

    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(container->layout());
    if (!layout)
        return;

    // Build a map of card name -> widget
    QMap<QString, QWidget*> cardMap;
    for (int i = 0; i < cards.size(); i++) {
        if (!cards[i]->objectName().isEmpty())
            cardMap[cards[i]->objectName()] = cards[i];
    }

    // Find layout indices of non-card items (alerts label, spacers, etc.)
    // We only rearrange the card items, keeping everything else in place

    // Collect current card layout items in order
    QList<QPair<int, QLayoutItem*>> cardItems;
    for (int i = layout->count() - 1; i >= 0; i--) {
        QWidget *w = layout->itemAt(i)->widget();
        if (w && cards.contains(w)) {
            cardItems.prepend(qMakePair(i, layout->takeAt(i)));
        }
    }

    // Re-insert cards in saved order
    // Find the first insertion point (where first card was)
    int insertPoint = 0;
    if (!cardItems.isEmpty())
        insertPoint = cardItems.first().first;

    // First, insert cards that appear in saved order
    int pos = insertPoint;
    for (int i = 0; i < order.size(); i++) {
        QString name = order[i];
        for (int j = 0; j < cardItems.size(); j++) {
            QWidget *w = cardItems[j].second->widget();
            if (w && w->objectName() == name) {
                layout->insertItem(pos, cardItems[j].second);
                cardItems.removeAt(j);
                pos++;
                break;
            }
        }
    }

    // Insert any remaining cards that weren't in saved order
    for (int j = 0; j < cardItems.size(); j++) {
        layout->insertItem(pos, cardItems[j].second);
        pos++;
    }
}
