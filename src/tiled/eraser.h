/*
 * eraser.h
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

#ifndef ERASER_H
#define ERASER_H

#include "abstracttiletool.h"

namespace Tiled {
namespace Internal {

class MapObjectItem;

/**
 * Implements a simple eraser tool.
 */
class Eraser : public AbstractTileTool
{
    Q_OBJECT

public:
    Eraser(QObject *parent = 0);

    void activate(MapScene *scene);

    void mousePressed(QGraphicsSceneMouseEvent *event);
    void mouseReleased(QGraphicsSceneMouseEvent *event);
    void mouseMoved(const QPointF &pos, Qt::KeyboardModifiers modifiers);

    void languageChanged();

    void updateEnabledState();

protected:
    void tilePositionChanged(const QPoint &tilePos);
    MapObjectItem *topMostObjectItemAt(QPointF pos) const;

private:
    void doErase(bool mergeable);
    void doEraseItem(const QPointF &pos);

    bool mErasing;
    MapScene *mMapScene;
};

} // namespace Internal
} // namespace Tiled

#endif // ERASER_H
