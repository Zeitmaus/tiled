/*
 * eraser.cpp
 * Copyright 2009-2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "eraser.h"

#include "addremovemapobject.h"
#include "brushitem.h"
#include "erasetiles.h"
#include "map.h"
#include "mapdocument.h"
#include "mapobjectitem.h"
#include "mapscene.h"
#include "tilelayer.h"

using namespace Tiled;
using namespace Tiled::Internal;

Eraser::Eraser(QObject *parent)
    : AbstractTileTool(tr("Eraser"),
                       QIcon(QLatin1String(
                               ":images/22x22/stock-tool-eraser.png")),
                       QKeySequence(tr("E")),
                       parent)
    , mErasing(false)
    , mMapScene(0)
{
}

void Eraser::tilePositionChanged(const QPoint &tilePos)
{
    if (currentTileLayer()) {
        brushItem()->setTileRegion(QRect(tilePos, QSize(1, 1)));

        if (mErasing)
            doErase(true);
    }
}

void Eraser::mouseMoved(const QPointF &pos, Qt::KeyboardModifiers modifiers)
{
    AbstractTileTool::mouseMoved(pos, modifiers);
    if (!currentTileLayer() && mErasing)
        doEraseItem(pos);
}

void Eraser::mousePressed(QGraphicsSceneMouseEvent *event)
{
    if (currentTileLayer() && !brushItem()->isVisible())
        return;

    if (event->button() == Qt::LeftButton) {
        mErasing = true;
        doErase(false);

        if (!currentTileLayer())
            doEraseItem(event->scenePos());
    }
}

void Eraser::mouseReleased(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        mErasing = false;
}

void Eraser::languageChanged()
{
    setName(tr("Eraser"));
    setShortcut(QKeySequence(tr("E")));
}

void Eraser::doErase(bool mergeable)
{
    TileLayer *tileLayer = currentTileLayer();
    const QPoint tilePos = tilePosition();

    if (!tileLayer || !tileLayer->bounds().contains(tilePos))
        return;

    QRegion eraseRegion(tilePos.x(), tilePos.y(), 1, 1);
    EraseTiles *erase = new EraseTiles(mapDocument(), tileLayer, eraseRegion);
    erase->setMergeable(mergeable);

    mapDocument()->undoStack()->push(erase);
    mapDocument()->emitRegionEdited(eraseRegion, tileLayer);
}

void Eraser::activate(MapScene *mapScene)
{
    AbstractTileTool::activate(mapScene);
    mMapScene = mapScene;
}

void Eraser::doEraseItem(const QPointF &pos)
{
    foreach (QGraphicsItem *item, mMapScene->items(pos)) {
        if (MapObjectItem *objectItem = dynamic_cast<MapObjectItem*>(item))
        {
            RemoveMapObject *erase = new RemoveMapObject(mapDocument(),
                                                         objectItem->mapObject());
            mapDocument()->undoStack()->push(erase);
            return;
        }
    }
}

void Eraser::updateEnabledState()
{
    setEnabled(true);
}
