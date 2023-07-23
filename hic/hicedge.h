
#pragma once

#include "../graph/debruijnnode.h"
#include "../graph/graphicsitemedgecommon.h"

class HiCEdge
{
public:
    //CREATORS
    HiCEdge(DeBruijnNode * startingNode, DeBruijnNode * endingNode, int weight);

    DeBruijnNode * getStartingNode() const {return m_startingNode;}
    DeBruijnNode * getEndingNode() const {return m_endingNode;}
    GraphicsItemHiCEdge * getGraphicsItemEdge() const {return m_graphicsItemEdge;}
    void setGraphicsItemEdge(GraphicsItemHiCEdge * gie) {m_graphicsItemEdge = gie;}
    int getWeight() const {return m_weight;}
    bool isDrawn() const {return m_drawn;}
    bool isVisibleWithNode() const;
    bool isVisible() const;
    bool determineIfDrawn();
    void reset() {m_graphicsItemEdge = nullptr; m_drawn = false;}
    DeBruijnNode * getOtherNode(const DeBruijnNode * node) const;
    void setReverseComplement(HiCEdge * rc) {m_reverseComplement = rc;}
    HiCEdge * getReverseComplement(){return m_reverseComplement;}

private:
    DeBruijnNode * m_startingNode;
    DeBruijnNode * m_endingNode;
    GraphicsItemHiCEdge * m_graphicsItemEdge;
    bool m_drawn : 1;
    int m_weight;
    HiCEdge * m_reverseComplement;
};

