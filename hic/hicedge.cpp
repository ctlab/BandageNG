#include "hicedge.h"

#include "program/globals.h"
#include "program/settings.h"
#include "hic/hicmanager.h"

HiCEdge::HiCEdge(DeBruijnNode * startingNode, DeBruijnNode * endingNode, int weight) :
    m_startingNode(startingNode), m_endingNode(endingNode), m_graphicsItemEdge(nullptr), m_drawn(false), m_weight(weight)
{}


bool HiCEdge::determineIfDrawn() {
    if (!g_settings->doubleMode && getReverseComplement()->isDrawn()) {
        return (m_drawn = false);
    }
    return (m_drawn = isVisibleWithNode());
}

//This function determines whether the edge should be drawn to the screen.
bool HiCEdge::isVisible() const
{
  return g_hicManager->isDrawn(this);
}

bool HiCEdge::isVisibleWithNode() const
{
  return g_hicManager->isDrawnWithNode(this);
}

DeBruijnNode * HiCEdge::getOtherNode(const DeBruijnNode * node) const
{
  return node == m_startingNode ?  m_endingNode : m_startingNode;
}
