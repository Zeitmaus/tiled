/*
 * CreateTileObjectTool.cpp
 * Copyright 2010-2011, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "createtileobjecttool.h"

#include "addremovemapobject.h"
#include "map.h"
#include "mapdocument.h"
#include "mapobject.h"
#include "mapobjectitem.h"
#include "maprenderer.h"
#include "mapscene.h"
#include "objectgroup.h"
#include "objectgroupitem.h"
#include "preferences.h"
#include "tile.h"
#include "utils.h"

#include <QApplication>
#include <QPalette>

using namespace Tiled;
using namespace Tiled::Internal;

CreateTileObjectTool::CreateTileObjectTool(QObject *parent)
    : AbstractObjectTool(QString(),
          QIcon(QLatin1String(":images/24x24/insert-object.png")),
          QKeySequence(tr("O")),
          parent)
    , mNewMapObjectItem(0)
    , mOverlayObjectGroup(0)
    , mTile(0)
{
    setIcon(QIcon(QLatin1String(":images/24x24/insert-image.png")));
    Utils::setThemeIcon(this, "insert-image");
    languageChanged();
}

CreateTileObjectTool::~CreateTileObjectTool()
{
    delete mOverlayObjectGroup;
}

void CreateTileObjectTool::deactivate(MapScene *scene)
{
    if (mNewMapObjectItem) cancelNewMapObject();

    AbstractObjectTool::deactivate(scene);
}

void CreateTileObjectTool::mouseEntered()
{
}

void CreateTileObjectTool::mouseMoved(const QPointF &pos,
                                  Qt::KeyboardModifiers modifiers)
{
    AbstractObjectTool::mouseMoved(pos, modifiers);

    if (!mNewMapObjectItem) return;

    const MapRenderer *renderer = mapDocument()->renderer();

    bool snapToGrid = Preferences::instance()->snapToGrid();
    if (modifiers & Qt::ControlModifier)
        snapToGrid = !snapToGrid;

    const QSize imgSize = mNewMapObjectItem->mapObject()->tile()->size();
    const QPointF diff(-imgSize.width() / 2, imgSize.height() / 2);
    QPointF tileCoords = renderer->pixelToTileCoords(pos + diff);

    if (snapToGrid)
        tileCoords = tileCoords.toPoint();

    mNewMapObjectItem->mapObject()->setPosition(tileCoords);
    mNewMapObjectItem->syncWithMapObject();
}

void CreateTileObjectTool::mousePressed(QGraphicsSceneMouseEvent *event)
{
    // Check if we are already creating a new map object
    if (mNewMapObjectItem) {
        if (event->button() == Qt::RightButton)
            cancelNewMapObject();
        return;
    }

    if (event->button() != Qt::LeftButton) {
        AbstractObjectTool::mousePressed(event);
        return;
    }

    ObjectGroup *objectGroup = currentObjectGroup();
    if (!objectGroup || !objectGroup->isVisible())
        return;

    if (!mTile) return;

    const MapRenderer *renderer = mapDocument()->renderer();
    const QPointF diff(-mTile->width() / 2, mTile->height() / 2);
    QPointF tileCoords = renderer->pixelToTileCoords(event->scenePos() + diff);

    bool snapToGrid = Preferences::instance()->snapToGrid();
    if (event->modifiers() & Qt::ControlModifier)
        snapToGrid = !snapToGrid;

    if (snapToGrid)
        tileCoords = tileCoords.toPoint();

    startNewMapObject(tileCoords, objectGroup);
}

void CreateTileObjectTool::mouseReleased(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && mNewMapObjectItem)
        finishNewMapObject();
}

void CreateTileObjectTool::languageChanged()
{
    setName(tr("Insert Tile"));
    setShortcut(QKeySequence(tr("T")));
}

void CreateTileObjectTool::startNewMapObject(const QPointF &pos,
                                         ObjectGroup *objectGroup)
{
    Q_ASSERT(!mNewMapObjectItem);

    if (!mTile) return;

    MapObject *newMapObject = new MapObject;
    newMapObject->setPosition(pos);
    newMapObject->setTile(mTile);
    objectGroup->addObject(newMapObject);

    mNewMapObjectItem = new MapObjectItem(newMapObject, mapDocument());
    mapScene()->addItem(mNewMapObjectItem);
}

MapObject *CreateTileObjectTool::clearNewMapObjectItem()
{
    Q_ASSERT(mNewMapObjectItem);

    MapObject *newMapObject = mNewMapObjectItem->mapObject();

    ObjectGroup *objectGroup = newMapObject->objectGroup();
    objectGroup->removeObject(newMapObject);

    delete mNewMapObjectItem;
    mNewMapObjectItem = 0;

    return newMapObject;
}

void CreateTileObjectTool::cancelNewMapObject()
{
    MapObject *newMapObject = clearNewMapObjectItem();
    delete newMapObject;
}

void CreateTileObjectTool::finishNewMapObject()
{
    Q_ASSERT(mNewMapObjectItem);
    MapObject *newMapObject = mNewMapObjectItem->mapObject();
    ObjectGroup *objectGroup = newMapObject->objectGroup();
    clearNewMapObjectItem();

    mapDocument()->undoStack()->push(new AddMapObject(mapDocument(),
                                                      objectGroup,
                                                      newMapObject));
}
