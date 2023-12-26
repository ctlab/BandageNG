#include "textgraphicsitemnode.h"
#include "program/globals.h"
#include "program/settings.h"

#include "ui/bandagegraphicsscene.h"

TextGraphicsItemNode::TextGraphicsItemNode(QString text,
                                   const QPointF centre,
                                   QGraphicsItem *parent)
        : CommonGraphicsItemNode(parent), m_text(text), m_centre(centre)
{
}

QRectF TextGraphicsItemNode::boundingRect() const
{
    auto text = shape();
    text.translate(m_centre);

    QRectF bound = text.boundingRect();
    bound.setTop(bound.top() - 0.5);
    bound.setBottom(bound.bottom() + 0.5);
    bound.setLeft(bound.left() - 0.5);
    bound.setRight(bound.right() + 0.5);

    return bound;
}

QPainterPath TextGraphicsItemNode::shape() const
{
    QPainterPath textPath;

    QFont font = g_settings->labelFont;
    font.setBold(true);

    QFontMetrics metrics(font);
    double fontHeight = metrics.ascent();

    int stepsUntilLast = m_text.size() - 1;
    double shiftLeft = -metrics.boundingRect(m_text).width() / 2.0;
    textPath.addText(0, -fontHeight, font, m_text);

    return textPath;
}


void TextGraphicsItemNode::paint(QPainter * painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QPainterPath textPath = shape();
    QRectF textBoundingRect = textPath.boundingRect();
    double textHeight = textBoundingRect.height();
    QPointF offset(0.0, textHeight / 2.0);

    double zoom = g_absoluteZoom;
    if (zoom == 0.0)
        zoom = 1.0;

    double zoomAdjustment = 1.0 / (1.0 + ((zoom - 1.0) * g_settings->textZoomScaleFactor));
    double inverseZoomAdjustment = 1.0 / zoomAdjustment;

    painter->translate(m_centre);
    painter->rotate(-g_graphicsView->getRotation());
    painter->scale(zoomAdjustment, zoomAdjustment);
    painter->translate(offset);

//    painter->setBrush(Qt::NoBrush);
//    painter->setPen(QPen(Qt::black, 1.0));
//    painter->drawRect(boundingRect());

    drawTextPathAtLocation(painter, textPath);
    painter->translate(-offset);
    painter->scale(inverseZoomAdjustment, inverseZoomAdjustment);
    painter->rotate(g_graphicsView->getRotation());
    painter->translate(-m_centre);
}

void TextGraphicsItemNode::drawTextPathAtLocation(QPainter * painter, const QPainterPath &textPath)
{
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
}

void TextGraphicsItemNode::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    //updateGrabIndex(event);
}

//When this node graphics item is moved, each of the connected edge
//graphics items will need to be adjusted accordingly.
void TextGraphicsItemNode::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
        QPointF difference = event->pos() - event->lastPos();
        auto *graphicsScene = dynamic_cast<BandageGraphicsScene *>(scene());


        m_centre += difference;
        graphicsScene->possiblyExpandSceneRectangle(this);
}
