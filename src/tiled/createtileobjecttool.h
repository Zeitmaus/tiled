#ifndef CREATETILEOBJECTTOOL_H
#define CREATETILEOBJECTTOOL_H

#include "abstractobjecttool.h"

namespace Tiled {

class Tile;

namespace Internal {

class MapObjectItem;

class CreateTileObjectTool : public AbstractObjectTool
{
    Q_OBJECT

public:
    CreateTileObjectTool(QObject *parent = 0);
    ~CreateTileObjectTool();

    void activate(MapScene *scene);
    void deactivate(MapScene *scene);

    void mouseEntered();
    void mouseMoved(const QPointF &pos,
                    Qt::KeyboardModifiers modifiers);
    void mousePressed(QGraphicsSceneMouseEvent *event);
    void mouseReleased(QGraphicsSceneMouseEvent *event);

    void languageChanged();

    void flipHorizontally();
    void flipVertically();
    void rotateCW();

public slots:
    /**
     * Sets the tile that will be used when the creation mode is
     * CreateTileObjects.
     */
    void setTile(Tile *tile);

private:
    void startNewMapObject();
    MapObject *clearNewMapObjectItem();
    void cancelNewMapObject();
    void finishNewMapObject();

    MapObjectItem *mNewMapObjectItem;
    ObjectGroup *mOverlayObjectGroup;
    Tile *mTile;
};

} // namespace Internal
} // namespace Tiled

#endif // CREATETILEOBJECTTOOL_H
