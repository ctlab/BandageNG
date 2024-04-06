//Copyright 2017 Ryan Wick

//This file is part of Bandage

//Bandage is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//Bandage is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

//You should have received a copy of the GNU General Public License
//along with Bandage.  If not, see <http://www.gnu.org/licenses/>.


#include "graphicsitemnode.h"
#include "graphicsitemedgecommon.h"
#include "debruijnnode.h"
#include "debruijnedge.h"
#include "assemblygraph.h"
#include "annotationsmanager.h"

#include "painting/textgraphicsitemnode.h"

#include "program/globals.h"
#include "program/memory.h"
#include "program/settings.h"

#include "ui/bandagegraphicsscene.h"
#include "ui/bandagegraphicsview.h"

#include <QTransform>
#include <QPainterPathStroker>
#include <QPainter>
#include <QPen>
#include <QMessageBox>
#include <QFontMetrics>

#include <QSize>

#include <set>

#include <cmath>
#include <cstdlib>
#include <utility>

#include "small_vector/small_pod_vector.hpp"

//This constructor makes a new GraphicsItemNode by copying the line points of
//the given node.
GraphicsItemNode::GraphicsItemNode(DeBruijnNode * deBruijnNode,
                                   GraphicsItemNode * toCopy,
                                   QGraphicsItem * parent) :
    CommonGraphicsItemNode(parent), m_deBruijnNode(deBruijnNode) {
    m_linePoints = toCopy->m_linePoints;
    m_colour = toCopy->m_colour;
    m_width = toCopy->m_width;
    m_grabIndex = toCopy->m_grabIndex;
    m_hasArrow = toCopy->m_hasArrow;
    remakePath();
}

// This constructor makes a new GraphicsItemNode with a specific collection of
// line points.
GraphicsItemNode::GraphicsItemNode(DeBruijnNode *deBruijnNode,
                                   double depthRelativeToMeanDrawnDepth,
                                   const std::vector<QPointF> &linePoints,
                                   QGraphicsItem *parent)
        : CommonGraphicsItemNode(parent), m_deBruijnNode(deBruijnNode)
{
    m_width = 0;
    m_grabIndex = 0;
    m_hasArrow = g_settings->doubleMode || g_settings->arrowheadsInSingleMode;
    m_linePoints.assign(linePoints.begin(), linePoints.end());
    setWidth(depthRelativeToMeanDrawnDepth);
    remakePath();
}

GraphicsItemNode::GraphicsItemNode(DeBruijnNode *deBruijnNode,
                                   double depthRelativeToMeanDrawnDepth,
                                   const adt::SmallPODVector<QPointF> &linePoints,
                                   QGraphicsItem *parent)
        : CommonGraphicsItemNode(parent), m_deBruijnNode(deBruijnNode)
{
    m_width = 0;
    m_grabIndex = 0;
    m_hasArrow = g_settings->doubleMode || g_settings->arrowheadsInSingleMode;
    m_linePoints.assign(linePoints.begin(), linePoints.end());
    setWidth(depthRelativeToMeanDrawnDepth);
    remakePath();
}

float GraphicsItemNode::getNodeWidth(double depthRelativeToMeanDrawnDepth, double depthPower,
                                     double depthEffectOnWidth, double averageNodeWidth) {
    if (depthRelativeToMeanDrawnDepth < 0.0)
        depthRelativeToMeanDrawnDepth = 0.0;
    double widthRelativeToAverage = (pow(depthRelativeToMeanDrawnDepth, depthPower) - 1.0) * depthEffectOnWidth + 1.0;
    return float(averageNodeWidth * widthRelativeToAverage);
}

void GraphicsItemNode::setWidth(double depthRelativeToMeanDrawnDepth, double averageNodeWidth,
                                double depthPower, double depthEffectOnWidth) {
    m_width = getNodeWidth(depthRelativeToMeanDrawnDepth,
                           depthPower, depthEffectOnWidth, averageNodeWidth);
    if (m_width < 0.0)
        m_width = 0.0;
}

static bool anyNodeDisplayText() {
    return g_settings->displayNodeCustomLabels ||
           g_settings->displayNodeNames ||
           g_settings->displayNodeLengths ||
           g_settings->displayNodeDepth ||
           g_settings->displayNodeCsvData;
}

void GraphicsItemNode::paint(QPainter * painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    static AnnotationGroup::AnnotationVector emptyAnnotations{};

    //This code lets me see the node's bounding box.
    //I use it for debugging graphics issues.
//    painter->setBrush(Qt::NoBrush);
//    painter->setPen(QPen(Qt::black, 1.0));
//    painter->drawRect(boundingRect());

    QPainterPath outlinePath = shape();

    //Fill the node's colour
    QBrush brush(m_colour);
    painter->fillPath(outlinePath, brush);

    //If the node has an arrow, then it's necessary to use the outline
    //as a clipping path so the colours don't extend past the edge of the
    //node.
    if (m_hasArrow)
        painter->setClipPath(outlinePath);

    for (const auto &annotationGroup : g_annotationsManager->getGroups()) {
        auto annotationSettings = g_settings->annotationsSettings[annotationGroup->id];

        const auto &annotations = annotationGroup->getAnnotations(m_deBruijnNode);
        const auto &revCompAnnotations = g_settings->doubleMode
                                         ? emptyAnnotations
                                         : annotationGroup->getAnnotations(m_deBruijnNode->getReverseComplement());

        for (const auto &annotation : annotations) {
            annotation->drawFigure(*painter, *this, false, annotationSettings.viewsToShow);
        }
        for (const auto &annotation : revCompAnnotations) {
            annotation->drawFigure(*painter, *this, true, annotationSettings.viewsToShow);
        }
    }
    painter->setClipping(false);

    //Draw the node outline
    QColor outlineColour = g_settings->outlineColour;
    double outlineThickness = g_settings->outlineThickness;
    if (isSelected())
    {
        outlineColour = g_settings->selectionColour;
        outlineThickness = g_settings->selectionThickness;
    }
    if (outlineThickness > 0.0)
    {
        outlinePath = outlinePath.simplified();
        QPen outlinePen(QBrush(outlineColour), outlineThickness, Qt::SolidLine,
                        Qt::SquareCap, Qt::RoundJoin);
        painter->setPen(outlinePen);
        painter->drawPath(outlinePath);
    }


    // raw the path highlighting outline, if appropriate
    if (g_memory->pathDialogIsVisible)
        exactPathHighlightNode(painter);

    // Draw the query path, if appropriate
    if (g_memory->queryPathDialogIsVisible)
        queryPathHighlightNode(painter);

    //Draw node labels if there are any to display.
    if (anyNodeDisplayText())
    {
          QStringList nodeText = getNodeText();
          QPainterPath textPath;

          QFont font = g_settings->labelFont;

          QFontMetrics metrics(font);
          double fontHeight = metrics.ascent();

          for (int i = 0; i < nodeText.size(); ++i)
          {
              const QString& text = nodeText.at(i);
              int stepsUntilLast = nodeText.size() - 1 - i;
              double shiftLeft = -metrics.boundingRect(text).width() / 2.0;
              textPath.addText(shiftLeft, -stepsUntilLast * fontHeight, font, text);
          }

          std::vector<QPointF> centres;
          if (g_settings->positionTextNodeCentre)
              centres.push_back(getCentre<adt::SmallPODVector<QPointF>>(m_linePoints));
          else
              centres = getCentres();

          for (auto &centre : centres)
              drawTextPathAtLocation(painter, textPath, centre);
    }

    //Draw BLAST hit labels, if appropriate.
    for (const auto &annotationGroup : g_annotationsManager->getGroups()) {
        if (!g_settings->annotationsSettings[annotationGroup->id].showText)
            continue;

        const auto &annotations = annotationGroup->getAnnotations(m_deBruijnNode);
        const auto &revCompAnnotations = g_settings->doubleMode
                                         ? emptyAnnotations
                                         : annotationGroup->getAnnotations(m_deBruijnNode->getReverseComplement());

        for (const auto &annotation : annotations) {
            annotation->drawDescription(*painter, *this, false);
        }
        for (const auto &annotation : revCompAnnotations) {
            annotation->drawDescription(*painter, *this, true);
        }
    }
}

void GraphicsItemNode::drawTextPathAtLocation(QPainter * painter, const QPainterPath &textPath, QPointF centre)
{
    QRectF textBoundingRect = textPath.boundingRect();
    double textHeight = textBoundingRect.height();
    QPointF offset(0.0, textHeight / 2.0);

    double zoom = g_absoluteZoom;
    if (zoom == 0.0)
        zoom = 1.0;

    double zoomAdjustment = 1.0 / (1.0 + ((zoom - 1.0) * g_settings->textZoomScaleFactor));
    double inverseZoomAdjustment = 1.0 / zoomAdjustment;

    painter->translate(centre);
    painter->rotate(-g_graphicsView->getRotation());
    painter->scale(zoomAdjustment, zoomAdjustment);
    painter->translate(offset);

    if (g_settings->textOutline)
    {
        painter->setPen(QPen(g_settings->textOutlineColour,
                             g_settings->textOutlineThickness * 2.0,
                             Qt::SolidLine,
                             Qt::SquareCap,
                             Qt::RoundJoin));
        painter->drawPath(textPath);
    }

    painter->fillPath(textPath, QBrush(g_settings->textColour));
    painter->translate(-offset);
    painter->scale(inverseZoomAdjustment, inverseZoomAdjustment);
    painter->rotate(g_graphicsView->getRotation());
    painter->translate(-centre);
}

QPainterPath GraphicsItemNode::shape() const
{

    //If there is only one segment, and it is shorter than half its
    //width, then the arrow head will not be made with 45 degree
    //angles, but rather whatever angle is made by going from the
    //end to the back corners (the final node will be a triangle).
    if (m_hasArrow && m_linePoints.size() == 2 &&
        distance(getLast(), getSecondLast()) < m_width / 2.0)
    {
        QLineF backline = QLineF(getSecondLast(), getLast()).normalVector();
        backline.setLength(m_width / 2.0);
        QPointF backVector = backline.p2() - backline.p1();
        QPainterPath trianglePath;
        trianglePath.moveTo(getLast());
        trianglePath.lineTo(getSecondLast() + backVector);
        trianglePath.lineTo(getSecondLast() - backVector);
        trianglePath.lineTo(getLast());
        return trianglePath;
    }

    //Create a path that outlines the main node shape.
    QPainterPathStroker stroker;
    stroker.setWidth(m_width);
    stroker.setCapStyle(Qt::FlatCap);
    stroker.setJoinStyle(Qt::RoundJoin);
    QPainterPath mainNodePath = stroker.createStroke(m_path);

    if (!m_hasArrow)
        return mainNodePath;

    //If the node has an arrow head, subtract the part of its
    //final segment to give it a pointy end.
    //NOTE: THIS APPROACH CAN LEAD TO WEIRD EFFECTS WHEN THE NODE'S
    //POINTY END OVERLAPS WITH ANOTHER PART OF THE NODE.  PERHAPS THERE
    //IS A BETTER WAY TO MAKE ARROWHEADS?
    QLineF frontLine = QLineF(getLast(), getSecondLast()).normalVector();
    frontLine.setLength(m_width / 2.0);
    QPointF frontVector = frontLine.p2() - frontLine.p1();
    QLineF arrowheadLine(getLast(), getSecondLast());
    arrowheadLine.setLength(1.42 * (m_width / 2.0));
    arrowheadLine.setAngle(arrowheadLine.angle() + 45.0);
    QPointF arrow1 = arrowheadLine.p2();
    arrowheadLine.setAngle(arrowheadLine.angle() - 90.0);
    QPointF arrow2 = arrowheadLine.p2();
    QLineF lastSegmentLine(getSecondLast(), getLast());
    lastSegmentLine.setLength(0.01);
    QPointF additionalForwardBit = lastSegmentLine.p2() - lastSegmentLine.p1();
    QPainterPath subtractionPath;
    subtractionPath.moveTo(getLast());
    subtractionPath.lineTo(arrow1);
    subtractionPath.lineTo(getLast() + frontVector + additionalForwardBit);
    subtractionPath.lineTo(getLast() - frontVector + additionalForwardBit);
    subtractionPath.lineTo(arrow2);
    subtractionPath.lineTo(getLast());

    QPainterPath mainNodePathTmp = mainNodePath.subtracted(subtractionPath);

    QLineF backLine = QLineF(getFirst(), getSecond()).normalVector();
    backLine.setLength(m_width / 2.0);
    QPointF backVector = backLine.p2() - backLine.p1();
    QLineF arrowBackLine(getSecond(), getFirst());
    arrowBackLine.setLength(m_width / 2.0);
    QPointF arrowBackVector = arrowBackLine.p2() - arrowBackLine.p1();
    QPainterPath addedPath;
    addedPath.moveTo(getFirst());
    addedPath.lineTo(getFirst() + backVector + arrowBackVector);
    addedPath.lineTo(getFirst() + backVector);
    addedPath.lineTo(getFirst() - backVector);
    addedPath.lineTo(getFirst() - backVector + arrowBackVector);
    addedPath.lineTo(getFirst());
    mainNodePathTmp.addPath(addedPath);

    return mainNodePathTmp;
}


void GraphicsItemNode::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    updateGrabIndex(event);
}


//When this node graphics item is moved, each of the connected edge
//graphics items will need to be adjusted accordingly.
void GraphicsItemNode::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    if (g_settings->roundMode) {
            QPointF lastPos = event->lastPos();
            QPointF newPos = event->pos();
            QPointF centralPos;
            if (m_grabIndex > m_linePoints.size() / 2) {
                centralPos = m_linePoints[0];
            }
            else {
                centralPos = m_linePoints[m_linePoints.size() - 1];
            }

            BandageGraphicsScene* graphicsScene = dynamic_cast<BandageGraphicsScene*>(scene());

            std::vector<GraphicsItemNode*> nodesToMove;
            nodesToMove.push_back(this);
            double alpha = angleBetweenTwoLines(centralPos, lastPos, centralPos, newPos);
            roundPoints(centralPos, alpha);
            remakePath();
            graphicsScene->possiblyExpandSceneRectangle(&nodesToMove);

            fixEdgePaths(&nodesToMove);
            fixHiCEdgePaths(&nodesToMove);
        }
    else {
        QPointF difference = event->pos() - event->lastPos();

        //If this node is selected, then move all of the other selected nodes too.
        //If it is not selected, then only move this node.
        std::vector<GraphicsItemNode *> nodesToMove;
        auto *graphicsScene = dynamic_cast<BandageGraphicsScene *>(scene());
        if (isSelected())
            nodesToMove = graphicsScene->getSelectedGraphicsItemNodes();
        else
            nodesToMove.push_back(this);

        bool allNodesFromOneGraph = true;
        int graphId = -1;
        for (auto &node : nodesToMove)
        {
            node->shiftPoints(difference);
            node->remakePath();
            if (graphId != -1 && allNodesFromOneGraph && node->m_deBruijnNode->getGraphId() != graphId) {
                allNodesFromOneGraph == false;
            }
            graphId = node->m_deBruijnNode->getGraphId();
        }
        graphicsScene->possiblyExpandSceneRectangle(&nodesToMove);

        fixEdgePaths(&nodesToMove);
        fixHiCEdgePaths(&nodesToMove);
        AssemblyGraph& currrentGraph = *g_assemblyGraph->m_graphMap[graphId];
        if (g_settings->multyGraphMode && nodesToMove.size() == currrentGraph.getDrawnNodeCount()) {
            if (currrentGraph.hasTextGraphicsItem()) {
                TextGraphicsItemNode * graphTitle = currrentGraph.getTextGraphicsItemNode();
                graphTitle->mouseMoveEvent(event);
            }
        }
    }
}


// This function remakes edge paths.  If nodes is passed, it will remake the
// edge paths for all of the nodes.  If nodes isn't passed, then it will just
// do it for this node.
void GraphicsItemNode::fixEdgePaths(std::vector<GraphicsItemNode *> * nodes) const {
    std::set<DeBruijnEdge *> edgesToFix;

    if (nodes == nullptr) {
        for (auto *edge : m_deBruijnNode->edges())
            edgesToFix.insert(edge);
    } else {
        for (auto &graphicNode : *nodes) {
            DeBruijnNode * node = graphicNode->m_deBruijnNode;
            for (auto *edge : node->edges())
                edgesToFix.insert(edge);
        }
    }

    for (auto *deBruijnEdge : edgesToFix) {
        //If this edge has a graphics item, adjust it now.
        if (GraphicsItemEdge *graphicsItemEdge = deBruijnEdge->getGraphicsItemEdge())
            graphicsItemEdge->remakePath();

        // If this edge does not have a graphics item, then perhaps its
        // reverse complement does.  Only do this check if the graph was drawn
        // on single mode.
        else if (!g_settings->doubleMode) {
            if (GraphicsItemEdge *graphicsItemEdge = deBruijnEdge->getReverseComplement()->getGraphicsItemEdge())
                graphicsItemEdge->remakePath();
        }
    }
}

// This function remakes edge paths.  If nodes is passed, it will remake the
// edge paths for all of the nodes.  If nodes isn't passed, then it will just
// do it for this node.
void GraphicsItemNode::fixHiCEdgePaths(std::vector<GraphicsItemNode *> * nodes) const {
    std::set<HiCEdge *> edgesToFix;

    if (nodes == nullptr) {
        for (auto *edge : m_deBruijnNode->hicEdges())
            edgesToFix.insert(edge);
    } else {
        for (auto &graphicNode : *nodes) {
            DeBruijnNode * node = graphicNode->m_deBruijnNode;
            for (auto *edge : node->hicEdges())
                edgesToFix.insert(edge);
        }
    }

    for (auto *hicEdge : edgesToFix) {
        //If this edge has a graphics item, adjust it now.
        if (GraphicsItemHiCEdge *graphicsItemEdge = hicEdge->getGraphicsItemEdge())
            graphicsItemEdge->remakePath();
    }
}


bool GraphicsItemNode::usePositiveNodeColour() const
{
    return !m_hasArrow || m_deBruijnNode->isPositiveNode();
}

QStringList GraphicsItemNode::getNodeText() const
{
    QStringList nodeText;

    if (g_settings->displayNodeCustomLabels) {
        nodeText << g_assemblyGraph->getCustomLabelForDisplay(m_deBruijnNode);
    }
    if (g_settings->displayNodeNames)
    {
        QString nodeName = m_deBruijnNode->getName();
        if (!g_settings->doubleMode)
            nodeName.chop(1);
        nodeText << nodeName;
    }
    if (g_settings->displayNodeLengths)
        nodeText << formatIntForDisplay(m_deBruijnNode->getLength()) + " bp";
    if (g_settings->displayNodeDepth)
        nodeText << formatDepthForDisplay(m_deBruijnNode->getDepth());
    if (g_settings->displayNodeCsvData) {
        for (auto graph : g_assemblyGraph->m_graphMap.values()) {
            auto data = graph->getCsvLine(m_deBruijnNode, g_settings->displayNodeCsvDataCol);
            if (data)
                nodeText << *data;
        }
    }
    return nodeText;
}


QSize GraphicsItemNode::getNodeTextSize(const QString& text)
{
    QFontMetrics fontMetrics(g_settings->labelFont);
    return fontMetrics.size(0, text);
}

//The bounding rectangle of a node has to be a little bit bigger than
//the node's path, because of the outline.  The selection outline is
//the largest outline we can expect, so use that to define the bounding
//rectangle.
QRectF GraphicsItemNode::boundingRect() const
{
    double extraSize = g_settings->selectionThickness / 2.0;
    QRectF bound = shape().boundingRect();

    bound.setTop(bound.top() - extraSize);
    bound.setBottom(bound.bottom() + extraSize);
    bound.setLeft(bound.left() - extraSize);
    bound.setRight(bound.right() + extraSize);

    return bound;
}


double GraphicsItemNode::indexToFraction(int64_t pos) const {
    return static_cast<double>(pos) / m_deBruijnNode->getLength();
}


//This function outlines and shades the appropriate part of a node if it is
//in the user-specified path.
void GraphicsItemNode::exactPathHighlightNode(QPainter * painter)
{
    if (g_memory->userSpecifiedPath.containsNode(m_deBruijnNode))
        pathHighlightNode2(painter, m_deBruijnNode, false, &g_memory->userSpecifiedPath);

    if (!g_settings->doubleMode &&
            g_memory->userSpecifiedPath.containsNode(m_deBruijnNode->getReverseComplement()))
        pathHighlightNode2(painter, m_deBruijnNode->getReverseComplement(), true, &g_memory->userSpecifiedPath);
}


//This function outlines and shades the appropriate part of a node if it is
//in the user-specified path.
void GraphicsItemNode::queryPathHighlightNode(QPainter * painter)
{
    if (g_memory->queryPaths.empty())
        return;

    for (auto &queryPath : g_memory->queryPaths)
    {
        Path * path = &queryPath;
        if (path->containsNode(m_deBruijnNode))
            pathHighlightNode2(painter, m_deBruijnNode, false, path);

        if (!g_settings->doubleMode &&
                path->containsNode(m_deBruijnNode->getReverseComplement()))
            pathHighlightNode2(painter, m_deBruijnNode->getReverseComplement(), true, path);
    }
}

static void pathHighlightNode3(QPainter * painter,
                               QPainterPath highlightPath) {
    QBrush shadingBrush(g_settings->pathHighlightShadingColour);
    painter->fillPath(highlightPath, shadingBrush);

    highlightPath = highlightPath.simplified();
    QPen outlinePen(QBrush(g_settings->pathHighlightOutlineColour),
                    g_settings->selectionThickness, Qt::SolidLine,
                    Qt::SquareCap, Qt::RoundJoin);
    painter->setPen(outlinePen);
    painter->drawPath(highlightPath);
}

void GraphicsItemNode::pathHighlightNode2(QPainter * painter,
                                          DeBruijnNode * node,
                                          bool reverse,
                                          Path * path)
{
    int numberOfTimesInMiddle = path->numberOfOccurrencesInMiddleOfPath(node);
    for (int i = 0; i < numberOfTimesInMiddle; ++i)
        pathHighlightNode3(painter, shape());

    bool isStartingNode = path->isStartingNode(node);
    bool isEndingNode = path->isEndingNode(node);

    //If this is the only node in the path, then we limit the highlighting to the appropriate region.
    if (isStartingNode && isEndingNode && path->getNodeCount() == 1)
    {
        pathHighlightNode3(painter, buildPartialHighlightPath(path->getStartFraction(), path->getEndFraction(), reverse));
        return;
    }

    if (isStartingNode)
        pathHighlightNode3(painter, buildPartialHighlightPath(path->getStartFraction(), 1.0, reverse));

    if (isEndingNode)
        pathHighlightNode3(painter, buildPartialHighlightPath(0.0, path->getEndFraction(), reverse));
}

QPainterPath GraphicsItemNode::buildPartialHighlightPath(double startFraction,
                                                         double endFraction,
                                                         bool reverse)
{
    if (reverse)
    {
        startFraction = 1.0 - startFraction;
        endFraction = 1.0 - endFraction;
        std::swap(startFraction, endFraction);
    }

    QPainterPath partialPath = CommonGraphicsItemNode::makePartialPath(startFraction, endFraction);

    QPainterPathStroker stroker;

    //If the node has an arrow, we need a path intersection with the
    //shape to make sure the arrowhead is part of the path.  Adding a bit
    //to the width seems to help with the intersection.
    if (m_hasArrow)
        stroker.setWidth(m_width + 0.1);
    else
        stroker.setWidth(m_width);

    stroker.setCapStyle(Qt::FlatCap);
    stroker.setJoinStyle(Qt::RoundJoin);
    QPainterPath highlightPath = stroker.createStroke(partialPath);

    if (m_hasArrow)
        highlightPath = highlightPath.intersected(shape());

    return highlightPath;
}
