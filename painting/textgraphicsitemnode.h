#ifndef TEXTGRAPHICSITEMNODE_H
#define TEXTGRAPHICSITEMNODE_H

#include "painting/commongraphicsitemnode.h"

class TextGraphicsItemNode : public CommonGraphicsItemNode
{
public:
    TextGraphicsItemNode(QString text,
                         const QPointF centre,
                         QGraphicsItem * parent = nullptr);
    const QPointF m_centre;
    QString m_text;
    void paint(QPainter * painter, const QStyleOptionGraphicsItem *, QWidget *) override;

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void drawTextPathAtLocation(QPainter * painter, const QPainterPath &textPath);
    void mousePressEvent(QGraphicsSceneMouseEvent * event);
};
#endif // TEXTGRAPHICSITEMNODE_H
