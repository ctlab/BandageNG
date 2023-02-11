#ifndef GRAPHICSITEMFUTUREEDGE_H
#define GRAPHICSITEMFUTUREEDGE_H

#include "featuretreenode.h"
#include <QGraphicsItem>

class GraphicsItemFeatureNode;

class GraphicsItemFeatureEdge : public QGraphicsPathItem
{
public:
    GraphicsItemFeatureEdge(FeatureTreeNode* startingNode, FeatureTreeNode* endingNode, QGraphicsItem* parent = 0);

    FeatureTreeNode* m_startingNode;
    FeatureTreeNode* m_endingNode;
    QPointF m_startingLocation;
    QPointF m_endingLocation;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*);
    void calculateAndSetPath();

private:
};
#endif //GRAPHICSITEMFUTUREEDGE_H
