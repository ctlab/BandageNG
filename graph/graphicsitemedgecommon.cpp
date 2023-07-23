#include "graphicsitemedgecommon.h"
#include "graphicsitemnode.h"
#include "debruijnedge.h"
#include "debruijnnode.h"

#include "hic/hicedge.h"
#include "graph/assemblygraph.h"
#include "program/globals.h"
#include "program/settings.h"

#include <QPainterPathStroker>
#include <QGraphicsPathItem>
#include <QPainter>
#include <QPen>
#include <QLineF>
#include <QBrush>

GraphicsItemEdgeCommon::GraphicsItemEdgeCommon(QGraphicsItem * parent)
    : QGraphicsPathItem(parent)
{}

static QPointF extendLine(QPointF start, QPointF end, double extensionLength) {
    double extensionRatio = extensionLength / QLineF(start, end).length();
    QPointF difference = end - start;
    difference *= extensionRatio;
    return end + difference;
}

// This function handles the special case of an edge that connects a node
// to itself where the node graphics item has only one line segment.
static void makeSpecialPathConnectingNodeToSelf(QPainterPath &path,
                                                const QPointF &startLocation, const QPointF &beforeStartLocation,
                                                const QPointF &endLocation, const QPointF &afterEndLocation) {
    double extensionLength = g_settings->edgeLength;
    QPointF controlPoint1 = extendLine(beforeStartLocation, startLocation, extensionLength),
            controlPoint2 = extendLine(afterEndLocation, endLocation, extensionLength);

    QLineF nodeLine(startLocation, endLocation);
    QLineF normalUnitLine = nodeLine.normalVector().unitVector();
    QPointF perpendicularShift = (normalUnitLine.p2() - normalUnitLine.p1()) * g_settings->edgeLength;
    QPointF nodeMidPoint = (startLocation + endLocation) / 2.0;

    path.moveTo(startLocation);

    path.cubicTo(controlPoint1, controlPoint1 + perpendicularShift, nodeMidPoint + perpendicularShift);
    path.cubicTo(controlPoint2 + perpendicularShift, controlPoint2, endLocation);
}

// This function handles the special case of an edge that connects a node to its
// reverse complement and is displayed in single mode.
static void makeSpecialPathConnectingNodeToReverseComplement(QPainterPath &path,
                                                             const QPointF &startLocation, const QPointF &beforeStartLocation,
                                                             const QPointF &endLocation, const QPointF &afterEndLocation) {
    double extensionLength = g_settings->edgeLength / 2.0;
    QPointF controlPoint1 = extendLine(beforeStartLocation, startLocation, extensionLength),
            controlPoint2 = extendLine(afterEndLocation, endLocation, extensionLength);

    QPointF startToControl = controlPoint1 - startLocation;
    QPointF pathMidPoint = startLocation + startToControl * 3.0;

    QLineF normalLine = QLineF(controlPoint1, startLocation).normalVector();
    QPointF perpendicularShift = (normalLine.p2() - normalLine.p1()) * 1.5;

    path.moveTo(startLocation);

    path.cubicTo(controlPoint1, pathMidPoint + perpendicularShift, pathMidPoint);
    path.cubicTo(pathMidPoint - perpendicularShift, controlPoint2, endLocation);
}


void GraphicsItemEdgeCommon::calculateAndSetPath(DeBruijnNode* startingNode, DeBruijnNode* endingNode, QPointF startLocation, QPointF beforeStartLocation, QPointF endLocation, QPointF afterEndLocation) {

    double edgeDistance = QLineF(startLocation, endLocation).length();

    double extensionLength = g_settings->edgeLength;
    if (extensionLength > edgeDistance / 2.0)
        extensionLength = edgeDistance / 2.0;

    QPointF controlPoint1 = extendLine(beforeStartLocation, startLocation, extensionLength),
            controlPoint2 = extendLine(afterEndLocation, endLocation, extensionLength);

    QPainterPath path;

    // If this edge is connecting a node to itself, and that node
    // is made of only one line segment, then a special path is
    // required, otherwise the edge will be mostly hidden underneath
    // the node.
    if (startingNode == endingNode) {
        GraphicsItemNode * graphicsItemNode = startingNode->getGraphicsItemNode();
        if (!graphicsItemNode)
            graphicsItemNode = startingNode->getReverseComplement()->getGraphicsItemNode();
        if (graphicsItemNode && graphicsItemNode->m_linePoints.size() == 2)
            makeSpecialPathConnectingNodeToSelf(path,
                                                startLocation, beforeStartLocation,
                                                endLocation, afterEndLocation);
    } else if (startingNode == endingNode->getReverseComplement() &&
               !g_settings->doubleMode) {
        //If we are in single mode and the edge connects a node to its reverse
        //complement, then we need a special path to make it visible.
        makeSpecialPathConnectingNodeToReverseComplement(path,
                                                         startLocation, beforeStartLocation,
                                                         endLocation, afterEndLocation);
    } else {
        // Otherwise, the path is just a single cubic Bezier curve.
        path.moveTo(startLocation);
        path.cubicTo(controlPoint1, controlPoint2, endLocation);
    }
    setPath(path);
}

GraphicsItemEdge::GraphicsItemEdge(DeBruijnEdge * deBruijnEdge, QGraphicsItem * parent)
    : GraphicsItemEdgeCommon(parent), m_deBruijnEdge(deBruijnEdge) {
    m_edgeColor = g_assemblyGraph->getCustomColour(deBruijnEdge);
    auto style = g_assemblyGraph->getCustomStyle(deBruijnEdge);
    m_penStyle = style.lineStyle;
    m_width = style.width;

    remakePath();
}

void GraphicsItemEdge::paint(QPainter * painter, const QStyleOptionGraphicsItem *, QWidget *) {
    QColor penColour = isSelected() ? g_settings->selectionColour : m_edgeColor;
    QPen edgePen(QBrush(penColour), m_width, m_penStyle, Qt::RoundCap);
    painter->setPen(edgePen);
    painter->drawPath(path());
}

void GraphicsItemEdge::remakePath() {
    QPointF startLocation, beforeStartLocation, endLocation, afterEndLocation;
    getControlPointLocations(m_deBruijnEdge,
                             startLocation, beforeStartLocation,
                             endLocation, afterEndLocation);
    GraphicsItemEdgeCommon::calculateAndSetPath(
                m_deBruijnEdge->getStartingNode(),
                m_deBruijnEdge->getEndingNode(),
                startLocation,
                beforeStartLocation,
                endLocation,
                afterEndLocation);
}

void GraphicsItemEdge::getControlPointLocations(const DeBruijnEdge *edge,
                                     QPointF &startLocation, QPointF &beforeStartLocation,
                                     QPointF &endLocation, QPointF &afterEndLocation) {
    DeBruijnNode * startingNode = edge->getStartingNode();
    DeBruijnNode * endingNode = edge->getEndingNode();

    if (startingNode->hasGraphicsItem()) {
        startLocation = startingNode->getGraphicsItemNode()->getLast();
        beforeStartLocation = startingNode->getGraphicsItemNode()->getSecondLast();
    } else if (startingNode->getReverseComplement()->hasGraphicsItem()) {
        startLocation = startingNode->getReverseComplement()->getGraphicsItemNode()->getFirst();
        beforeStartLocation = startingNode->getReverseComplement()->getGraphicsItemNode()->getSecond();
    }

    if (endingNode->hasGraphicsItem()) {
        endLocation = endingNode->getGraphicsItemNode()->getFirst();
        afterEndLocation = endingNode->getGraphicsItemNode()->getSecond();
    } else if (endingNode->getReverseComplement()->hasGraphicsItem()) {
        endLocation = endingNode->getReverseComplement()->getGraphicsItemNode()->getLast();
        afterEndLocation = endingNode->getReverseComplement()->getGraphicsItemNode()->getSecondLast();
    }
}

QPainterPath GraphicsItemEdge::shape() const {
    QPainterPathStroker stroker;
    stroker.setWidth(m_width);
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);
    return stroker.createStroke(path());
}

GraphicsItemHiCEdge::GraphicsItemHiCEdge(int maxWeight, HiCEdge* edge, QGraphicsItem* parent):
    GraphicsItemEdgeCommon(parent), m_edge(edge), m_maxWeight(maxWeight) {
    remakePath();
}

void GraphicsItemHiCEdge::paint(QPainter * painter, const QStyleOptionGraphicsItem *, QWidget *) {
    int dark = 200 - 200 * (log(m_edge->getWeight()) / log(m_maxWeight));
    QColor penColour;
    penColour.setRgb(dark, dark, dark);
    QPen edgePen(QBrush(penColour), g_settings->edgeWidth, Qt::DotLine, Qt::RoundCap);
    painter->setPen(edgePen);
    painter->drawPath(path());
}

QPainterPath GraphicsItemHiCEdge::shape() const {
    QPainterPathStroker stroker;
    stroker.setWidth(g_settings->edgeWidth);
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);
    return stroker.createStroke(path());
}

void GraphicsItemHiCEdge::remakePath() {
    QPointF startLocation, beforeStartLocation, endLocation, afterEndLocation;
    getControlPointLocations(m_edge,
                             startLocation, beforeStartLocation,
                             endLocation, afterEndLocation);
    GraphicsItemEdgeCommon::calculateAndSetPath(m_edge->getStartingNode(),
                                                m_edge->getEndingNode(),
                                                startLocation,
                                                beforeStartLocation,
                                                endLocation,
                                                afterEndLocation);
}

void GraphicsItemHiCEdge::getControlPointLocations(const HiCEdge *edge,
                                     QPointF &startLocation, QPointF &beforeStartLocation,
                                     QPointF &endLocation, QPointF &afterEndLocation) {
    DeBruijnNode * startingNode = edge->getStartingNode();
    DeBruijnNode * endingNode = edge->getEndingNode();

    if (startingNode->hasGraphicsItem())
    {
        if (startingNode->getGraphicsItemNode()->isBig()) {
            startLocation = startingNode->getGraphicsItemNode()->getMiddle();
            beforeStartLocation = startingNode->getGraphicsItemNode()->getBeforeMiddle();
        }
        else {
            startLocation = startingNode->getGraphicsItemNode()->getLast();
            beforeStartLocation = startingNode->getGraphicsItemNode()->getSecondLast();
        }
    }
    else if (startingNode->getReverseComplement()->hasGraphicsItem())
    {
        if (startingNode->getReverseComplement() -> getGraphicsItemNode()->isBig()) {
            startLocation = startingNode->getReverseComplement() -> getGraphicsItemNode()->getMiddle();
            beforeStartLocation = startingNode->getReverseComplement() -> getGraphicsItemNode()->getAfterMiddle();
        }
        else {
            startLocation = startingNode->getReverseComplement()->getGraphicsItemNode()->getFirst();
            beforeStartLocation = startingNode->getReverseComplement()->getGraphicsItemNode()->getSecond();
        }
    }

    if (endingNode->hasGraphicsItem())
    {
        if (endingNode->getGraphicsItemNode()->isBig()) {
            endLocation = endingNode->getGraphicsItemNode()->getMiddle();
            afterEndLocation = endingNode->getGraphicsItemNode()->getAfterMiddle();
        }
        else {
            endLocation = endingNode->getGraphicsItemNode()->getFirst();
            afterEndLocation = endingNode->getGraphicsItemNode()->getSecond();
        }
    }
    else if (endingNode->getReverseComplement()->hasGraphicsItem())
    {
        if (endingNode->getReverseComplement()->getGraphicsItemNode()->isBig()) {
            endLocation = endingNode->getReverseComplement()->getGraphicsItemNode()->getMiddle();
            afterEndLocation = endingNode->getReverseComplement()->getGraphicsItemNode()->getBeforeMiddle();
        }
        else {
            endLocation = endingNode->getReverseComplement()->getGraphicsItemNode()->getLast();
            afterEndLocation = endingNode->getReverseComplement()->getGraphicsItemNode()->getSecondLast();
        }
    }
}
