#include "graphicsitemfeaturenode.h"
#include <QPainter>
#include "featuretreenode.h"
#include "graphicsitemfeatureedge.h"
#include <QGraphicsScene>
#include "ui/bandagegraphicsview.h"
#include "ui/bandagegraphicsscene.h"
#include <set>
#include "featuretreeedge.h"
#include <iostream>

GraphicsItemFeatureNode::GraphicsItemFeatureNode(FeatureTreeNode* featureNode,
                                   const adt::SmallPODVector<QPointF> &linePoints,
                                   CommonGraphicsItemNode *parent)
        : CommonGraphicsItemNode(parent), m_featureNode(featureNode) {
    m_linePoints.assign(linePoints.begin(), linePoints.end());
    m_width = g_settings->averageFeatureNodeWidth;
    remakePath();
}

QRectF GraphicsItemFeatureNode::boundingRect() const
{
    //double extraSize = g_settings->selectionThickness / 2.0;
    QRectF bound = shape().boundingRect();

    bound.setTop(bound.top() - 0.5);
    bound.setBottom(bound.bottom() + 0.5);
    bound.setLeft(bound.left() - 0.5);
    bound.setRight(bound.right() + 0.5);

    return bound;
}

QPainterPath GraphicsItemFeatureNode::shape() const
{
    int width = g_settings->averageFeatureNodeWidth;
    QPainterPath mainNodePath;
    mainNodePath.addEllipse(m_linePoints[0].toPoint(), width, width);
    return mainNodePath;
}

QPainterPath GraphicsItemFeatureNode::shapeRect() const
{
    int width = g_settings->averageFeatureNodeWidth;
    QRect r(m_linePoints[0].toPoint().x(), m_linePoints[0].toPoint().y(), width, width);
    r.moveCenter(m_linePoints[0].toPoint());
    QPainterPath mainNodePath;
    mainNodePath.addRect(r);
    return mainNodePath;
}

void GraphicsItemFeatureNode::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    int width = g_settings->averageFeatureNodeWidth;
    if (width % 2 == 0) {
        width += 1;
    }
    int x = m_linePoints[0].x();
    int y = m_linePoints[0].y();

    QBrush brush;
    QColor outlineColour = g_settings->outlineColour;
    double outlineThickness = g_settings->outlineThickness;
    if (isSelected()) {
        outlineColour = g_settings->selectionColour;
        outlineThickness = g_settings->selectionThickness;
    }
    if (outlineThickness > 0.0)
    {
        QPen outlinePen(QBrush(outlineColour), outlineThickness, Qt::SolidLine,
            Qt::SquareCap, Qt::RoundJoin);
        painter->setPen(outlinePen);
    }
    painter->setBrush(m_colour);
    if (g_settings->displayFeatureClassLikeFigure) {
        if (m_featureNode->getClassInd() == 0) {
            QPainterPath outlinePath = shape();
            QRect r(x, y, width, width);
            r.moveCenter(m_linePoints[0].toPoint());
            painter->fillPath(outlinePath, brush);
            painter->drawEllipse(r);
        }
        else if (m_featureNode->getClassInd() == 1) {
            QRect r(x, y, width, width);
            r.moveCenter(m_linePoints[0].toPoint());
            
            QPainterPath outlinePath;
            outlinePath.addRect(r);
            painter->fillPath(outlinePath, brush);
            painter->drawRect(r);
        }
        else if (m_featureNode->getClassInd() == 2) {
            QRect r(x, y, width, width);
            r.moveCenter(m_linePoints[0].toPoint());

            QPainterPath outlinePath;
            outlinePath.moveTo(r.left() + (r.width() / 2), r.top());
            outlinePath.lineTo(r.bottomLeft());
            outlinePath.lineTo(r.bottomRight());
            outlinePath.lineTo(r.left() + (r.width() / 2), r.top());

            painter->fillPath(outlinePath, brush);
            painter->drawPath(outlinePath);
        } else if (m_featureNode->getClassInd() == 3) {
            QRect r(x, y, width, width);
            r.moveCenter(m_linePoints[0].toPoint());

            QPainterPath outlinePath;
            outlinePath.moveTo(r.left() + (r.width() / 2), r.bottom());
            outlinePath.lineTo(r.topLeft());
            outlinePath.lineTo(r.topRight());
            outlinePath.lineTo(r.left() + (r.width() / 2), r.bottom());

            painter->fillPath(outlinePath, brush);
            painter->drawPath(outlinePath);
        } else if (m_featureNode->getClassInd() == 4) {
            QRect r(x, y, width, width);
            r.moveCenter(m_linePoints[0].toPoint());
            QPainterPath outlinePath;
            outlinePath.moveTo(r.center().x(), r.top());
            outlinePath.lineTo(r.right(), r.center().y());
            outlinePath.lineTo(r.center().x(), r.bottom());
            outlinePath.lineTo(r.left(), r.center().y());
            outlinePath.lineTo(r.center().x(), r.top());

            painter->fillPath(outlinePath, brush);
            painter->drawPath(outlinePath);
        }
    }
    else {
        QPainterPath outlinePath = shape();
        QRect r(x, y, width, width);
        r.moveCenter(m_linePoints[0].toPoint());
        painter->fillPath(outlinePath, brush);
        painter->drawEllipse(r);
    }

    if (g_settings->displayFeatureIdLabels ||
        g_settings->displayFeatureClassLabels ||
        g_settings->displayFeatureCustomLabels)
    {
        QStringList nodeText = getNodeText();
        drawNodeText(painter, nodeText);
    }

    return;
}

void GraphicsItemFeatureNode::drawNodeText(QPainter* painter, QStringList nodeText) {
    QPainterPath textPath;

    QFont font = g_settings->labelFont;
    font.setPixelSize(14);
    QFontMetrics metrics(font);
    double fontHeight = metrics.ascent();

    for (int i = 0; i < nodeText.size(); ++i)
    {
        QString text = nodeText.at(i);
        int stepsUntilLast = nodeText.size() - 1 - i;
        double shiftLeft = -metrics.boundingRect(text).width() / 2.0 - 1.0;
        textPath.addText(shiftLeft, -stepsUntilLast * fontHeight, font, text);
    }

    drawTextPathAtLocation(painter, textPath);
}

void GraphicsItemFeatureNode::drawTextPathAtLocation(QPainter* painter, QPainterPath textPath)
{
    QRectF textBoundingRect = textPath.boundingRect();
    double textHeight = textBoundingRect.height();
    QPointF offset(0.0, textHeight / 2.0);
    QPointF centre(m_linePoints[0].x() - textBoundingRect.width() / 2.0 - m_width / 2.0 - 1.0, m_linePoints[0].y());

    double zoom = g_absoluteZoomFeatures;
    if (zoom == 0.0)
        zoom = 1.0;

    double zoomAdjustment = 1.0 / (1.0 + ((zoom - 1.0) * g_settings->textZoomScaleFactor));
    double inverseZoomAdjustment = 1.0 / zoomAdjustment;

    painter->translate(centre);
    painter->rotate(-g_graphicsViewFeaturesForest->getRotation());
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
    painter->rotate(g_graphicsViewFeaturesForest->getRotation());
    painter->translate(-centre);
}

QStringList GraphicsItemFeatureNode::getNodeText()
{
    QStringList nodeText;
    if (g_settings->displayFeatureCustomLabels)
        nodeText << m_featureNode->getCustomLabelForDisplay();
    if (g_settings->displayFeatureIdLabels)
    {
        QString id = m_featureNode->getName();
        if (id.size() != 0) {
            nodeText << id;
        }
        //nodeText << id;
    }
    if (g_settings->displayFeatureClassLabels)
        nodeText << m_featureNode->getClass();
    return nodeText;
}

void GraphicsItemFeatureNode::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    updateGrabIndex(event);
}

//When this node graphics item is moved, each of the connected edge
//graphics items will need to be adjusted accordingly.
void GraphicsItemFeatureNode::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QPointF difference = event->pos() - event->lastPos();

    //If this node is selected, then move all of the other selected nodes too.
    //If it is not selected, then only move this node.
    std::vector<GraphicsItemFeatureNode*> nodesToMove;
    BandageGraphicsScene* graphicsScene = dynamic_cast<BandageGraphicsScene*>(scene());
    if (isSelected())
        nodesToMove = graphicsScene->getSelectedGraphicsItemFeatureNode();
    else
        nodesToMove.push_back(this);

    for (size_t i = 0; i < nodesToMove.size(); ++i)
    {
        nodesToMove[i]->shiftPoints(difference);
        nodesToMove[i]->remakePath();
    }
    graphicsScene->possiblyExpandSceneRectangle(&nodesToMove);

    fixEdgePaths(&nodesToMove);
}

void GraphicsItemFeatureNode::fixEdgePaths(std::vector<GraphicsItemFeatureNode*>* nodes)
{
    std::set<FeatureTreeEdge*> edgesToFix;

    if (nodes == 0)
    {
        const std::vector<FeatureTreeEdge*>* edges = m_featureNode->getEdges();
        for (size_t j = 0; j < edges->size(); ++j)
            edgesToFix.insert((*edges)[j]);
    }
    else
    {
        for (size_t i = 0; i < nodes->size(); ++i)
        {
            FeatureTreeNode* node = (*nodes)[i]->m_featureNode;
            const std::vector<FeatureTreeEdge*>* edges = node->getEdges();
            for (size_t j = 0; j < edges->size(); ++j)
                edgesToFix.insert((*edges)[j]);
        }
    }

    for (std::set<FeatureTreeEdge*>::iterator i = edgesToFix.begin(); i != edgesToFix.end(); ++i)
    {
        FeatureTreeEdge* FeatureTreeEdge = *i;
        GraphicsItemFeatureEdge* graphicsItemEdge = FeatureTreeEdge->getGraphicsItemFeatureEdge();

        //If this edge has a graphics item, adjust it now.
        if (graphicsItemEdge != 0)
            graphicsItemEdge->calculateAndSetPath();
    }
}
