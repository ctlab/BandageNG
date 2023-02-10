
#include "graphicsitemfeatureedge.h"
#include "../program/settings.h"
#include "../program/globals.h"
#include <QPen>
#include <QPainter>
#include "featuretreenode.h"
#include "graphicsitemfeaturenode.h"


GraphicsItemFeatureEdge::GraphicsItemFeatureEdge(FeatureTreeNode* startingNode, FeatureTreeNode* endingNode, QGraphicsItem* parent) :
    QGraphicsPathItem(parent), m_startingNode(startingNode), m_endingNode(endingNode) {
    calculateAndSetPath();
}

void GraphicsItemFeatureEdge::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    double edgeWidth = g_settings->edgeWidth;
    QColor penColour;
    penColour = g_settings->edgeColour;
    Qt::PenStyle s = Qt::SolidLine;
    QPen edgePen(QBrush(penColour), edgeWidth, s, Qt::RoundCap);
    painter->setPen(edgePen);
    painter->drawPath(path());
}

void GraphicsItemFeatureEdge::calculateAndSetPath() {
    {
        m_startingLocation = m_startingNode->getGraphicsItemFeatureNode()->getFirst();
        m_endingLocation = m_endingNode->getGraphicsItemFeatureNode()->getFirst();

        QPainterPath path;
        path.moveTo(m_startingLocation);
        path.lineTo(m_endingLocation);
        setPath(path);
    }
}
