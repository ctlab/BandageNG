#ifndef GRAPHICSITEMEDGECOMMON_H
#define GRAPHICSITEMEDGECOMMON_H

#include <QGraphicsPathItem>

class DeBruijnEdge;
class DeBruijnNode;
class HiCEdge;

class GraphicsItemEdgeCommon : public QGraphicsPathItem {
public:
    explicit GraphicsItemEdgeCommon(QGraphicsItem * parent = nullptr);
    //virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem *, QWidget *) override;
    //virtual QPainterPath shape() const override;
    //virtual ~GraphicsItemEdgeCommon() = default;
    void calculateAndSetPath(DeBruijnNode* startingNode,
                                    DeBruijnNode* endingNode,
                                    QPointF startLocation,
                                    QPointF beforeStartLocation,
                                    QPointF endLocation,
                                    QPointF afterEndLocation);

};

class GraphicsItemEdge : public GraphicsItemEdgeCommon {
private:
    DeBruijnEdge* m_deBruijnEdge;
    QColor m_edgeColor;
    Qt::PenStyle m_penStyle;
    float m_width;
public:
    //using GraphicsItemEdgeCommon::GraphicsItemEdgeCommon;
    //using GraphicsItemEdgeCommon::calculateAndSetPath;

    explicit GraphicsItemEdge(DeBruijnEdge * deBruijnEdge, QGraphicsItem * parent = nullptr);
    void paint(QPainter * painter, const QStyleOptionGraphicsItem *, QWidget *) override;
    QPainterPath shape() const override;

    void remakePath();
    DeBruijnEdge *edge() const { return m_deBruijnEdge; }
    void getControlPointLocations(const DeBruijnEdge *edge,
                                  QPointF &startLocation,
                                  QPointF &beforeStartLocation,
                                  QPointF &endLocation,
                                  QPointF &afterEndLocation);
};

class GraphicsItemHiCEdge: public GraphicsItemEdgeCommon  {
public:
    //using GraphicsItemEdgeCommon::GraphicsItemEdgeCommon;
    //using GraphicsItemEdgeCommon::calculateAndSetPath;

    explicit GraphicsItemHiCEdge(int maxWeight, HiCEdge* edge, QGraphicsItem* parent = nullptr);
    void paint(QPainter * painter, const QStyleOptionGraphicsItem *, QWidget *) override;
    QPainterPath shape() const override;

    void remakePath();
    void getControlPointLocations(const HiCEdge *edge,
                                  QPointF &startLocation,
                                  QPointF &beforeStartLocation,
                                  QPointF &endLocation,
                                  QPointF &afterEndLocation);
private:
    HiCEdge* m_edge;
    int m_maxWeight;
};

#endif // GRAPHICSITEMEDGECOMMON_H
