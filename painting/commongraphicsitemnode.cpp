#include "commongraphicsitemnode.h"
#include "../program/settings.h"
#include "../program/globals.h"
#include <QPainterPathStroker>
#include "../program/settings.h"
#include <QPainter>
#include <QPen>
#include <QMessageBox>
#include <math.h>
#include <QFontMetrics>
#include <QSize>
#include <stdlib.h>
#include <QGraphicsScene>
#include "../ui/bandagegraphicsscene.h"
#include <set>
#include "../ui/bandagegraphicsview.h"
#include <QTransform>
#include <math.h>
#include <QFontMetrics>
#include "../program/memory.h"

CommonGraphicsItemNode::CommonGraphicsItemNode(QGraphicsItem* parent) : QGraphicsItem(parent) { }

double CommonGraphicsItemNode::distance(QPointF p1, QPointF p2) {
    auto xDiff = p1.x() - p2.x();
    auto yDiff = p1.y() - p2.y();
    return sqrt(xDiff * xDiff + yDiff * yDiff);
}

//This function returns the nodes' visible centres.  If the entire node is visible,
//then there is just one visible centre.  If none of the node is visible, then
//there are no visible centres.  If multiple parts of the node are visible, then there
//are multiple visible centres.
std::vector<QPointF> CommonGraphicsItemNode::getCentres() const
{
    std::vector<QPointF> centres;
    std::vector<QPointF> currentRun;

    QPointF lastP;
    bool lastPointVisible = false;

    for (size_t i = 0; i < m_linePoints.size(); ++i)
    {
        QPointF p = m_linePoints[i];
        bool pVisible = g_graphicsView->isPointVisible(p);

        //If this point is visible, but the last wasn't, a new run is started.
        if (pVisible && !lastPointVisible)
        {
            //If this is not the first point, then we need to find the intermediate
            //point that lies on the visible boundary and start the path with that.
            if (i > 0)
                currentRun.push_back(g_graphicsView->findIntersectionWithViewportBoundary(QLineF(p, lastP)));
            currentRun.push_back(p);
        }

        //If th last point is visible and this one is too, add it to the current run.
        else if (pVisible && lastPointVisible)
            currentRun.push_back(p);

        //If the last point is visible and this one isn't, then a run has ended.
        else if (!pVisible && lastPointVisible)
        {
            //We need to find the intermediate point that is on the visible boundary.
            currentRun.push_back(g_graphicsView->findIntersectionWithViewportBoundary(QLineF(p, lastP)));

            centres.push_back(getCentre(currentRun));
            currentRun.clear();
        }

        //If neither this point nor the last were visible, we still need to check whether
        //the line segment between them is.  If so, then then this may be a case where
        //we are really zoomed in (and so line segments are large compared to the scene rect).
        else if (i > 0 && !pVisible && !lastPointVisible)
        {
            bool success;
            QLineF v = g_graphicsView->findVisiblePartOfLine(QLineF(lastP, p), &success);
            if (success)
            {
                QPointF vCentre = QPointF((v.p1().x() + v.p2().x()) / 2.0, (v.p1().y() + v.p2().y()) / 2.0);
                centres.push_back(vCentre);
            }
        }

        lastPointVisible = pVisible;
        lastP = p;
    }

    //If there is a current run, add its centre
    if (!currentRun.empty())
        centres.push_back(getCentre(currentRun));

    return centres;
}

void CommonGraphicsItemNode::updateGrabIndex(QGraphicsSceneMouseEvent* event) {
    m_grabIndex = 0;
    QPointF grabPoint = event->pos();

    double closestPointDistance = distance(grabPoint, m_linePoints[0]);
    for (size_t i = 1; i < m_linePoints.size(); ++i)
    {
        double pointDistance = distance(grabPoint, m_linePoints[i]);
        if (pointDistance < closestPointDistance)
        {
            closestPointDistance = pointDistance;
            m_grabIndex = i;
        }
    }
}

void CommonGraphicsItemNode::shiftPoints(QPointF difference)
{
    prepareGeometryChange();

    if (g_settings->nodeDragging == NO_DRAGGING)
        return;

    else if (isSelected()) //Move all pieces for selected nodes
    {
        for (size_t i = 0; i < m_linePoints.size(); ++i)
            m_linePoints[i] += difference;
    }

    else if (g_settings->nodeDragging == ONE_PIECE)
        m_linePoints[m_grabIndex] += difference;

    else if (g_settings->nodeDragging == NEARBY_PIECES)
    {
        for (size_t i = 0; i < m_linePoints.size(); ++i)
        {
            int indexDistance = abs(int(i) - int(m_grabIndex));
            double dragStrength = pow(2.0, -1.0 * pow(double(indexDistance), 1.8) / g_settings->dragStrength); //constants chosen for dropoff of drag strength
            m_linePoints[i] += difference * dragStrength;
        }
    }
}

void CommonGraphicsItemNode::remakePath()
{
    QPainterPath path;

    if (m_linePoints.size() > 0) {
        path.moveTo(m_linePoints[0]);
        if (m_linePoints.size() <= 2 || !(g_settings->isHiCLoaded)) {
            for (size_t i = 1; i < m_linePoints.size(); ++i)
                path.lineTo(m_linePoints[i]);
        }
        else {
            int middleInd = m_linePoints.size() / 2;
            for (size_t i = 1; i < middleInd - 1; ++i)
                path.lineTo(m_linePoints[i]);
            path.quadTo(m_linePoints[middleInd], m_linePoints[middleInd + 1]);

            for (size_t i = middleInd + 1; i < m_linePoints.size(); ++i)
                path.lineTo(m_linePoints[i]);
        }
    }

    m_path = path;
}

double CommonGraphicsItemNode::getNodePathLength()
{
    double totalLength = 0.0;
    for (size_t i = 0; i < m_linePoints.size() - 1; ++i)
    {
        QLineF line(m_linePoints[i], m_linePoints[i + 1]);
        totalLength += line.length();
    }
    return totalLength;
}

QPainterPath CommonGraphicsItemNode::makePartialPath(double startFraction, double endFraction)
{
    if (endFraction < startFraction)
        std::swap(startFraction, endFraction);

    double totalLength = getNodePathLength();

    QPainterPath path;
    bool pathStarted = false;
    double lengthSoFar = 0.0;
    for (size_t i = 0; i < m_linePoints.size() - 1; ++i)
    {
        QPointF point1 = m_linePoints[i];
        QPointF point2 = m_linePoints[i + 1];
        QLineF line(point1, point2);

        double point1Fraction = lengthSoFar / totalLength;
        lengthSoFar += line.length();
        double point2Fraction = lengthSoFar / totalLength;

        //If the path hasn't yet begun and this segment is before
        //the starting fraction, do nothing.
        if (!pathStarted && point2Fraction < startFraction)
            continue;

        //If the path hasn't yet begun but this segment covers the starting
        //fraction, start the path now.
        if (!pathStarted && point2Fraction >= startFraction)
        {
            pathStarted = true;
            path.moveTo(findIntermediatePoint(point1, point2, point1Fraction, point2Fraction, startFraction));
        }

        //If the path is in progress and this segment hasn't yet reached the end,
        //just continue the path.
        if (pathStarted && point2Fraction < endFraction)
            path.lineTo(point2);

        //If the path is in progress and this segment passes the end, finish the line.
        if (pathStarted && point2Fraction >= endFraction)
        {
            path.lineTo(findIntermediatePoint(point1, point2, point1Fraction, point2Fraction, endFraction));
            return path;
        }
    }

    return path;
}


//This function will find the point that is a certain fraction of the way along the node's path.
QPointF CommonGraphicsItemNode::findLocationOnPath(double fraction)
{
    double totalLength = getNodePathLength();

    double lengthSoFar = 0.0;
    for (size_t i = 0; i < m_linePoints.size() - 1; ++i)
    {
        QPointF point1 = m_linePoints[i];
        QPointF point2 = m_linePoints[i + 1];
        QLineF line(point1, point2);

        double point1Fraction = lengthSoFar / totalLength;
        lengthSoFar += line.length();
        double point2Fraction = lengthSoFar / totalLength;

        //If point2 hasn't yet reached the target, do nothing.
        if (point2Fraction < fraction)
            continue;

        //If the path hasn't yet begun but this segment covers the starting
        //fraction, start the path now.
        if (point2Fraction >= fraction)
            return findIntermediatePoint(point1, point2, point1Fraction, point2Fraction, fraction);
    }

    //The code shouldn't get here, as the target point should have been found in the above loop.
    return {};
}

QPointF CommonGraphicsItemNode::findIntermediatePoint(QPointF p1, QPointF p2, double p1Value, double p2Value, double targetValue)
{
    QPointF difference = p2 - p1;
    double fraction = (targetValue - p1Value) / (p2Value - p1Value);
    return difference * fraction + p1;
}

//This function shifts all the node's points to the left (relative to its
//direction).  This is used in double mode to prevent nodes from displaying
//directly on top of their complement nodes.
void CommonGraphicsItemNode::shiftPointsLeft()
{
    shiftPointSideways(true);
}

void CommonGraphicsItemNode::shiftPointsRight()
{
    shiftPointSideways(false);
}

void CommonGraphicsItemNode::shiftPointSideways(bool left)
{
    prepareGeometryChange();

    //The collection of line points should be at least
    //two large.  But just to be safe, quit now if it
    //is not.
    size_t linePointsSize = m_linePoints.size();
    if (linePointsSize < 2)
        return;

    //Shift by a quarter of the segment length.  This should make
    //nodes one half segment length separated from their complements.
    double shiftDistance = g_settings->doubleModeNodeSeparation;

    for (size_t i = 0; i < linePointsSize; ++i)
    {
        QPointF point = m_linePoints[i];
        QLineF nodeDirection;

        //If the point is on the end, then determine the node direction
        //using this point and its adjacent point.
        if (i == 0)
        {
            QPointF nextPoint = m_linePoints[i + 1];
            nodeDirection = QLineF(point, nextPoint);
        }
        else if (i == linePointsSize - 1)
        {
            QPointF previousPoint = m_linePoints[i - 1];
            nodeDirection = QLineF(previousPoint, point);
        }

        // If the point is in the middle, then determine the node direction
        //using both adjacent points.
        else
        {
            QPointF previousPoint = m_linePoints[i - 1];
            QPointF nextPoint = m_linePoints[i + 1];
            nodeDirection = QLineF(previousPoint, nextPoint);
        }

        QLineF shiftLine = nodeDirection.normalVector().unitVector();
        shiftLine.setLength(shiftDistance);

        QPointF shiftVector;
        if (left)
            shiftVector = shiftLine.p2() - shiftLine.p1();
        else
            shiftVector = shiftLine.p1() - shiftLine.p2();
        QPointF newPoint = point + shiftVector;
        m_linePoints[i] = newPoint;
    }

    remakePath();
}

void CommonGraphicsItemNode::roundPoints(QPointF centralPoint, double alpha) {
    double sinA = sin(alpha);
    double cosA = cos(alpha);
    double xA = centralPoint.x();
    double yA = centralPoint.y();
    for (size_t i = 0; i < m_linePoints.size(); ++i) {
        QPointF point = m_linePoints[i];
        double x = -sinA * (point.y() - yA) + cosA * (point.x() - xA) + xA;
        double y = cosA * (point.y() - yA) + sinA * (point.x() - xA) + yA;
        point.setX(x);
        point.setY(y);
        m_linePoints[i] = point;
    }
}

double CommonGraphicsItemNode::angleBetweenTwoLines(QPointF line1Start, QPointF line1End, QPointF line2Start, QPointF line2End)
{
    double a = line1End.x() - line1Start.x();
    double b = line1End.y() - line1Start.y();
    double c = line2End.x() - line2Start.x();
    double d = line2End.y() - line2Start.y();

    double atanA = atan2(a, b);
    double atanB = atan2(c, d);

    return atanA - atanB;
}
