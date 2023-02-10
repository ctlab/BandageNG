#ifndef GRAPHICSITEMFUTURENODE_H
#define GRAPHICSITEMFUTURENODE_H

#include <QGraphicsSceneMouseEvent>
#include <vector>
#include <QPointF>
#include <QColor>
#include <QFont>
#include <QString>
#include <QPainterPath>
#include <QStringList>
#include <algorithm>
#include <iostream>
#include "../program/settings.h"
#include "../program/globals.h"
#include "../painting/commongraphicsitemnode.h"

class FeatureTreeNode;
class Path;

class GraphicsItemFeatureNode : public CommonGraphicsItemNode
{
public:
    GraphicsItemFeatureNode(FeatureTreeNode* featureNode,
                            const adt::SmallPODVector<QPointF> &linePoints,
                            CommonGraphicsItemNode *parent = 0);

    FeatureTreeNode* m_featureNode;

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*);

    QPainterPath shape() const;
    QPainterPath shapeRect() const;
    QPainterPath shapeTriangl() const;
    QStringList getNodeText();
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
private:
    void fixEdgePaths(std::vector<GraphicsItemFeatureNode*>* nodes);
    void drawNodeText(QPainter* painter, QStringList nodeText);
    void drawTextPathAtLocation(QPainter* painter, QPainterPath textPath);

};
#endif // GRAPHICSITEMFUTURENODE_H

