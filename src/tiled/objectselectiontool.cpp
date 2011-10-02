/*
 * objectselectiontool.cpp
 * Copyright 2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 *
 * This file is part of Tiled.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "objectselectiontool.h"

#include "layer.h"
#include "map.h"
#include "mapdocument.h"
#include "mapobject.h"
#include "mapobjectitem.h"
#include "maprenderer.h"
#include "mapscene.h"
#include "movemapobject.h"
#include "objectgroup.h"
#include "preferences.h"
#include "selectionrectangle.h"

#include <QApplication>
#include <QGraphicsItem>
#include <QUndoStack>
#include <QList>

using namespace Tiled;
using namespace Tiled::Internal;

ObjectSelectionTool::ObjectSelectionTool(QObject *parent)
    : AbstractObjectTool(tr("Select Objects"),
          QIcon(QLatin1String(":images/22x22/tool-select-objects.png")),
          QKeySequence(tr("S")),
          parent)
    , mSelectionRectangle(new SelectionRectangle)
    , mMousePressed(false)
    , mClickedObjectItem(0)
    , mMode(NoMode)
    , mLastSpecialItem(0)
{
}

ObjectSelectionTool::~ObjectSelectionTool()
{
    delete mSelectionRectangle;
}

void ObjectSelectionTool::mouseEntered()
{
}

void ObjectSelectionTool::mouseMoved(const QPointF &pos,
                                     Qt::KeyboardModifiers modifiers)
{
    AbstractObjectTool::mouseMoved(pos, modifiers);

    if (mMode == NoMode && mMousePressed) {
        const int dragDistance = (mStart - pos).manhattanLength();
        if (dragDistance >= QApplication::startDragDistance()) {
            if (mLastSpecialItem) {
                // Allows the user to sneak it out properly
                // after a special selection.  Again, we need
                // check if we're off in the void.
                if (objectItemInStack(pos, mLastSpecialItem))
                    mClickedObjectItem = mLastSpecialItem;
            }
            if (mClickedObjectItem)
                startMoving();
            else
                startSelecting();
        }
    }

    switch (mMode) {
    case Selecting:
        mSelectionRectangle->setRectangle(QRectF(mStart, pos).normalized());
        break;
    case Moving:
        updateMovingItems(pos, modifiers);
        break;
    case NoMode:
        break;
    }
}

void ObjectSelectionTool::mousePressed(QGraphicsSceneMouseEvent *event)
{
    if (mMode != NoMode) // Ignore additional presses during select/move
        return;

    switch (event->button()) {
    case Qt::LeftButton:
        mMousePressed = true;
        mStart = event->scenePos();
        if (event->modifiers() & Qt::AltModifier) {
            // A chunk of polish to keep the user experience solid.
            if (!mClickedObjectItem && !mLastSpecialItem) {
                QSet<MapObjectItem *> selSet = mapScene()->selectedObjectItems();
                if (selSet.count() == 1) {
                    QList<MapObjectItem *> selList = selSet.toList();
                    mLastSpecialItem = selList.at(0);
                }
            }
            mClickedObjectItem = sliceIncrement(event->scenePos());
        } else {
            mClickedObjectItem = topMostObjectItemAt(mStart);
        }
        break;
    case Qt::RightButton:
        if (mLastSpecialItem) {
            // IFF we've a valid value we need to ensure that we're
            // not popping the menu off in the void somewhere.
            QSet<MapObjectItem *> sel = mapScene()->selectedObjectItems();
            if (objectItemInStack(event->scenePos(),mLastSpecialItem)) {
                mClickedObjectItem = mLastSpecialItem;
            } else {
                // Since we're in the void, we need give the base class
                // something valid to chew on.
                mClickedObjectItem = topMostObjectItemAt(event->scenePos());
            }
            //Relies on behaviour in AbstractObjectTool
            sel.clear();
            if (mClickedObjectItem) sel.insert(mClickedObjectItem);
            mapScene()->setSelectedObjectItems(sel);

            // We *must* go around the default implementation or it will
            // get the idea that it ought grab a fresh selection by
            // default -- every time.
            showContextMenu(&*mClickedObjectItem, event->screenPos(), event->widget());
            return;
        }
    default:
        AbstractObjectTool::mousePressed(event);
        break;
    }
}

void ObjectSelectionTool::mouseReleased(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;

    // Ensure we clear the special selector on the first
    // invalid click
    if (!(event->modifiers() & Qt::AltModifier))
            mLastSpecialItem = 0;

    switch (mMode) {
    case NoMode:
        if (mClickedObjectItem) {
            QSet<MapObjectItem*> selection = mapScene()->selectedObjectItems();
            const Qt::KeyboardModifiers modifiers = event->modifiers();
            if (modifiers & (Qt::ShiftModifier | Qt::ControlModifier)) {
                if (selection.contains(mClickedObjectItem))
                    selection.remove(mClickedObjectItem);
                else
                    selection.insert(mClickedObjectItem);
            } else {
                selection.clear();
                selection.insert(mClickedObjectItem);
            }
            mapScene()->setSelectedObjectItems(selection);
        } else {
            mapScene()->setSelectedObjectItems(QSet<MapObjectItem*>());
        }
        break;
    case Selecting:
        updateSelection(event->scenePos(), event->modifiers());
        mapScene()->removeItem(mSelectionRectangle);
        mMode = NoMode;
        break;
    case Moving:
        finishMoving(event->scenePos());
        break;
    }

    mMousePressed = false;
    mClickedObjectItem = 0;
}

void ObjectSelectionTool::modifiersChanged(Qt::KeyboardModifiers modifiers)
{
    mModifiers = modifiers;
}

void ObjectSelectionTool::languageChanged()
{
    setName(tr("Select Objects"));
    setShortcut(QKeySequence(tr("S")));
}

void ObjectSelectionTool::updateSelection(const QPointF &pos,
                                          Qt::KeyboardModifiers modifiers)
{
    QRectF rect = QRectF(mStart, pos).normalized();

    // Make sure the rect has some contents, otherwise intersects returns false
    rect.setWidth(qMax(qreal(1), rect.width()));
    rect.setHeight(qMax(qreal(1), rect.height()));

    QSet<MapObjectItem*> selectedItems;

    foreach (QGraphicsItem *item, mapScene()->items(rect)) {
        MapObjectItem *mapObjectItem = dynamic_cast<MapObjectItem*>(item);
        if (mapObjectItem)
            selectedItems.insert(mapObjectItem);
    }

    const QSet<MapObjectItem*> oldSelection = mapScene()->selectedObjectItems();
    QSet<MapObjectItem*> newSelection;

    if (modifiers & (Qt::ControlModifier | Qt::ShiftModifier)) {
        newSelection = oldSelection | selectedItems;
    } else {
        newSelection = selectedItems;
    }

    mapScene()->setSelectedObjectItems(newSelection);
}

void ObjectSelectionTool::startSelecting()
{
    mMode = Selecting;
    mapScene()->addItem(mSelectionRectangle);
}

void ObjectSelectionTool::startMoving()
{
    mMovingItems = mapScene()->selectedObjectItems();

    // Move only the clicked item, if it was not part of the selection
    if (!mMovingItems.contains(mClickedObjectItem)) {
        mMovingItems.clear();
        mMovingItems.insert(mClickedObjectItem);
        mapScene()->setSelectedObjectItems(mMovingItems);
    }

    mMode = Moving;

    // Remember the current object positions
    mOldObjectItemPositions.clear();
    mOldObjectPositions.clear();
    mAlignPosition = (*mMovingItems.begin())->mapObject()->position();

    foreach (MapObjectItem *objectItem, mMovingItems) {
        const QPointF &pos = objectItem->mapObject()->position();
        mOldObjectItemPositions += objectItem->pos();
        mOldObjectPositions += pos;
        if (pos.x() < mAlignPosition.x())
            mAlignPosition.setX(pos.x());
        if (pos.y() < mAlignPosition.y())
            mAlignPosition.setY(pos.y());
    }
}

void ObjectSelectionTool::updateMovingItems(const QPointF &pos,
                                            Qt::KeyboardModifiers modifiers)
{
    MapRenderer *renderer = mapDocument()->renderer();
    QPointF diff = pos - mStart;

    bool snapToGrid = Preferences::instance()->snapToGrid();
    if (modifiers & Qt::ControlModifier)
        snapToGrid = !snapToGrid;

    if (snapToGrid) {
        const QPointF alignPixelPos =
                renderer->tileToPixelCoords(mAlignPosition);
        const QPointF newAlignPixelPos = alignPixelPos + diff;

        // Snap the position to the grid
        const QPointF newTileCoords =
                renderer->pixelToTileCoords(newAlignPixelPos).toPoint();
        diff = renderer->tileToPixelCoords(newTileCoords) - alignPixelPos;
    }

    int i = 0;
    foreach (MapObjectItem *objectItem, mMovingItems) {
        const QPointF newPixelPos = mOldObjectItemPositions.at(i) + diff;
        const QPointF newPos = renderer->pixelToTileCoords(newPixelPos);
        objectItem->setPos(newPixelPos);
        objectItem->setZValue(newPixelPos.y());
        objectItem->mapObject()->setPosition(newPos);
        ++i;
    }
}

void ObjectSelectionTool::finishMoving(const QPointF &pos)
{
    Q_ASSERT(mMode == Moving);
    mMode = NoMode;

    if (mStart == pos) // Move is a no-op
        return;

    QUndoStack *undoStack = mapDocument()->undoStack();
    undoStack->beginMacro(tr("Move %n Object(s)", "", mMovingItems.size()));
    int i = 0;
    foreach (MapObjectItem *objectItem, mMovingItems) {
        MapObject *object = objectItem->mapObject();
        const QPointF oldPos = mOldObjectPositions.at(i);
        undoStack->push(new MoveMapObject(mapDocument(), object, oldPos));
        ++i;
    }
    undoStack->endMacro();

    mOldObjectItemPositions.clear();
    mOldObjectPositions.clear();
    mMovingItems.clear();
}

MapObjectItem *ObjectSelectionTool::sliceIncrement(const QPointF &pos)
{
    QList<QGraphicsItem *> graphList = mapScene()->items(pos,
                                                         Qt::IntersectsItemShape,
                                                         Qt::DescendingOrder);
    QList<MapObjectItem *> moiList;
    ObjectGroup *curGroup = currentObjectGroup();

    foreach (QGraphicsItem *qgi, graphList) {
        //Get the list of stabbed QGraphicsItems in the
        //current GroupObject.
        MapObjectItem *moi = dynamic_cast<MapObjectItem *>(qgi);
        if (moi) {
            if (curGroup->objects().indexOf(moi->mapObject()) != -1)
                moiList.append(moi);
        }
    }

    if (!moiList.count()) return 0;

    int idx = moiList.indexOf(mLastSpecialItem) + 1;
    idx %= moiList.count();
    mLastSpecialItem = moiList.at(idx);
    return mLastSpecialItem;
}

bool ObjectSelectionTool::objectItemInStack(const QPointF &pos,
                                            MapObjectItem *objectItem)
{
    QGraphicsItem *tItem = dynamic_cast<QGraphicsItem *>(objectItem);
    QList<QGraphicsItem *> graphList = mapScene()->items(pos);
    int index = graphList.indexOf(tItem);
    return index >= 0;
}
