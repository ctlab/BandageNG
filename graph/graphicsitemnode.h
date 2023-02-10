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


#pragma once

#include "small_vector/small_pod_vector.hpp"

#include <QPointF>
#include <QColor>
#include <QFont>
#include <QString>
#include <QPainterPath>
#include <QStringList>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include "painting/commongraphicsitemnode.h"

#include <vector>

class DeBruijnNode;
class Path;

class GraphicsItemNode : public CommonGraphicsItemNode
{
public:
    GraphicsItemNode(DeBruijnNode * deBruijnNode,
                     GraphicsItemNode * toCopy,
                     QGraphicsItem * parent = nullptr);
    GraphicsItemNode(DeBruijnNode * deBruijnNode,
                     double depthRelativeToMeanDrawnDepth,
                     const std::vector<QPointF> &linePoints,
                     QGraphicsItem * parent = nullptr);
    GraphicsItemNode(DeBruijnNode * deBruijnNode,
                     double depthRelativeToMeanDrawnDepth,
                     const adt::SmallPODVector<QPointF> &linePoints,
                     QGraphicsItem * parent = nullptr);

    DeBruijnNode * m_deBruijnNode;

    static QSize getNodeTextSize(const QString& text);
    static float getNodeWidth(double depthRelativeToMeanDrawnDepth,
                              double depthPower,
                              double depthEffectOnWidth,
                              double averageNodeWidth);
    static void drawTextPathAtLocation(QPainter * painter, const QPainterPath &textPath, QPointF centre);

    void mousePressEvent(QGraphicsSceneMouseEvent * event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent * event) override;
    void paint(QPainter * painter, const QStyleOptionGraphicsItem *, QWidget *) override;
    QPainterPath shape() const override;
    bool usePositiveNodeColour() const;
    QStringList getNodeText() const;
    void setWidth(double depthRelativeToMeanDrawnDepth,
                  double averageNodeWidth = 5.0,
                  double depthPower = 0.5, double depthEffectOnWidth = 0.5);
    QRectF boundingRect() const override;
    void fixEdgePaths(std::vector<GraphicsItemNode *> * nodes = nullptr) const;
    double indexToFraction(int64_t pos) const;

private:
    void exactPathHighlightNode(QPainter * painter);
    void queryPathHighlightNode(QPainter * painter);
    void pathHighlightNode2(QPainter * painter, DeBruijnNode * node, bool reverse, Path * path);
    QPainterPath buildPartialHighlightPath(double startFraction, double endFraction, bool reverse);
};
