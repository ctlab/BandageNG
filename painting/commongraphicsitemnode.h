#ifndef COMMONGRAPHICSITEMNODE_H
#define COMMONGRAPHICSITEMNODE_H

#include "small_vector/small_pod_vector.hpp"

#include <QGraphicsItem>
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
#include "../ui/bandagegraphicsview.h"
#include "graph/assemblygraphlist.h"

class Path;

class CommonGraphicsItemNode : public QGraphicsItem
{
public:

    CommonGraphicsItemNode(QGraphicsItem* parent = 0);

    double m_width;
    bool m_hasArrow = 1;
    adt::SmallPODVector<QPointF> m_linePoints;
    size_t m_grabIndex = 31;
    QColor m_colour = QColor(178, 34, 34);
    QPainterPath m_path;

    static double distance(QPointF p1, QPointF p2);
    void shiftPoints(QPointF difference);
    void remakePath();
    void setWidth(double width) { m_width = width; }
    void updateGrabIndex(QGraphicsSceneMouseEvent* event);
    QPointF getFirst() const { return m_linePoints.front(); }
    QPointF getSecond() const {
        if (m_linePoints.size() > 1) return m_linePoints[1];
        else return m_linePoints[0];
    }
    QPointF getLast() const { return m_linePoints[m_linePoints.size() - 1]; }
    QPointF getSecondLast() const
    {
        if (m_linePoints.size() > 1) return m_linePoints[m_linePoints.size() - 2];
        else return m_linePoints.front();
    }
    qreal getMinX() const {
        if(m_linePoints.size() > 0) {
            qreal res = m_linePoints[0].x();
            for (auto point : m_linePoints)
                res = std::min(res, point.x());
            return res;
        }
        return 0;
    }
    qreal getMaxX() const {
        if(m_linePoints.size() > 0) {
            qreal res = m_linePoints[0].x();
            for (auto point : m_linePoints)
                res = std::max(res, point.x());
            return res;
        }
        return 0;
    }
    qreal getMinY() const {
        if(m_linePoints.size() > 0) {
            qreal res = m_linePoints[0].y();
            for (auto point : m_linePoints)
                res = std::min(res, point.y());
            return res;
        }
        return 0;
    }
    qreal getMaxY() const {
        if(m_linePoints.size() > 0) {
            qreal res = m_linePoints[0].y();
            for (auto point : m_linePoints)
                res = std::max(res, point.y());
            return res;
        }
        return 0;
    }
    bool isBig() const { return m_linePoints.size() >= 3; }
    bool isOne() const { return m_linePoints.size() == 1; }
    QPointF getMiddle() const { return m_linePoints[m_linePoints.size() / 2]; }
    QPointF getBeforeMiddle() const
    {
        if (m_linePoints.size() >= 3)
            return m_linePoints[(m_linePoints.size() / 2) - 1];
        else
            return m_linePoints[0];
    }
    QPointF getAfterMiddle() const
    {
        if (m_linePoints.size() >= 3)
            return m_linePoints[(m_linePoints.size() / 2) + 1];
        else
            return m_linePoints[m_linePoints.size() - 1];
    }
    std::vector<QPointF> getCentres() const;

    QPainterPath makePartialPath(double startFraction, double endFraction);
    double getNodePathLength();
    QPointF findLocationOnPath(double fraction);
    QPointF findIntermediatePoint(QPointF p1, QPointF p2, double p1Value,
        double p2Value, double targetValue);
    void shiftPointsLeft();
    void shiftPointsRight();
    void roundPoints(QPointF centralPoint, double alpha);
    double angleBetweenTwoLines(QPointF line1Start, QPointF line1End, QPointF line2Start, QPointF line2End);
    void setNodeColour(QColor color) { m_colour = color; }

    // This function finds the centre point on the path defined by linePoints.
    template<class Container>
    static QPointF getCentre(const Container &linePoints) {
        if (linePoints.empty())
            return {};
        if (linePoints.size() == 1)
            return linePoints[0];

        double pathLength = 0.0;
        for (size_t i = 0; i < linePoints.size() - 1; ++i)
            pathLength += distance(linePoints[i], linePoints[i+1]);

        double endToCentre = pathLength / 2.0;

        double lengthSoFar = 0.0;
        for (size_t i = 0; i < linePoints.size() - 1; ++i)
        {
            QPointF a = linePoints[i];
            QPointF b = linePoints[i+1];
            double segmentLength = distance(a, b);

            //If this segment will push the distance over halfway, then it
            //contains the centre point.
            if (lengthSoFar + segmentLength >= endToCentre)
            {
                double additionalLengthNeeded = endToCentre - lengthSoFar;
                double fractionOfCurrentSegment = additionalLengthNeeded / segmentLength;
                return (b - a) * fractionOfCurrentSegment + a;
            }

            lengthSoFar += segmentLength;
        }

        //Code should never get here.
        return {};
    }

private:
    void shiftPointSideways(bool left);
};

#endif //COMMONGRAPHICSITEMNODE_H

