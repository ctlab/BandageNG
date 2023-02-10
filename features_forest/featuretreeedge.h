#pragma once

#include "graphicsitemfeatureedge.h"
#include "featuretreenode.h"

class FeatureTreeEdge
{
public:
    FeatureTreeEdge(FeatureTreeNode *startingNode, FeatureTreeNode *endingNode);
    ~FeatureTreeEdge() = default;
    GraphicsItemFeatureEdge* getGraphicsItemFeatureEdge() {
		return m_graphicsEdge;
    }
    void setGraphicsItemFeatureEdge(GraphicsItemFeatureEdge* edge) { m_graphicsEdge = edge; }

    FeatureTreeNode * m_startingNode;
    FeatureTreeNode * m_endingNode;

private:
    GraphicsItemFeatureEdge* m_graphicsEdge;
};
