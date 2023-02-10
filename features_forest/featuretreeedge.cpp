#include "featuretreeedge.h"

FeatureTreeEdge::FeatureTreeEdge(FeatureTreeNode *startingNode, FeatureTreeNode *endingNode) :
    m_startingNode(startingNode), m_endingNode(endingNode), m_graphicsEdge(nullptr)
{
}
