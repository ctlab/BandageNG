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

#include "llvm/ADT/iterator_range.h"
#include "seq/sequence.hpp"
#include "small_vector/small_pod_vector.hpp"
//#include "graph/assemblygraph.h"

#include <QColor>
#include <QByteArray>
#include <vector>

class DeBruijnEdge;
class GraphicsItemNode;
class HiCEdge;
//class AssemblyGraph;

class DeBruijnNode
{
public:
    //CREATORS
    DeBruijnNode(int graphId, QString name, float depth, const Sequence &sequence, unsigned length = 0);
    ~DeBruijnNode() = default;

    //ACCESSORS
    QString getName() const {return m_name;}
    QString getNameWithoutSign() const {return m_name.left(m_name.length() - 1);}
    QString getSign() const {if (m_name.length() > 0) return m_name.right(1); else return "+";}

    double getDepth() const {return m_depth;}
    int getGraphId() const {return m_graphId;}

    float getGC() const;

    const Sequence &getSequence() const;
    Sequence &getSequence();

    unsigned getLength() const {return m_length;}
    unsigned getLengthWithoutTrailingOverlap() const;

    QByteArray getFasta(bool sign, bool newLines = true, bool evenIfEmpty = true) const;
    QByteArray getAAFasta(unsigned shift, bool sign, bool newLines, bool evenIfEmpty) const;

    char getBaseAt(int i) const {if (i >= 0 && i < m_sequence.size()) return m_sequence[i]; else return '\0';} // NOTE
    DeBruijnNode * getReverseComplement() const {return m_reverseComplement;}
    DeBruijnNode *getCanonical() { return isPositiveNode() ? this : m_reverseComplement; }

    GraphicsItemNode * getGraphicsItemNode() const {return m_graphicsItemNode;}
    bool hasGraphicsItem() const {return m_graphicsItemNode != nullptr;}

    auto edgeBegin() { return m_edges.begin(); }
    const auto edgeBegin() const { return m_edges.begin(); }
    auto edgeEnd() { return m_edges.end(); }
    const auto edgeEnd() const { return m_edges.end(); }
    auto edges() { return llvm::make_range(edgeBegin(), edgeEnd()); }
    const auto edges() const { return llvm::make_range(edgeBegin(), edgeEnd()); }

    const auto hicEdgeBegin() const { return m_hicEdges.begin(); }
    const auto hicEdgeEnd() const { return m_hicEdges.end(); }
    const auto hicEdges() const { return llvm::make_range(hicEdgeBegin(), hicEdgeEnd()); }

    std::vector<DeBruijnEdge *> getEnteringEdges() const;
    std::vector<DeBruijnEdge *> getLeavingEdges() const;
    std::vector<DeBruijnNode *> getDownstreamNodes() const;
    std::vector<DeBruijnNode *> getUpstreamNodes() const;
    std::vector<DeBruijnNode *> getAllConnectedPositiveNodes() const;
    bool isSpecialNode() const {return m_specialNode;}
    bool isDrawn() const {return m_drawn;}
    bool thisNodeOrReverseComplementIsDrawn() const {return isDrawn() || getReverseComplement()->isDrawn();}
    bool isNotDrawn() const {return !m_drawn;}
    bool isPositiveNode() const;
    bool isNegativeNode() const;

    bool isNodeConnected(DeBruijnNode * node) const;
    DeBruijnEdge * doesNodeLeadIn(DeBruijnNode * node) const;
    DeBruijnEdge * doesNodeLeadAway(DeBruijnNode * node) const;
    bool isInDepthRange(double min, double max) const;
    bool sequenceIsMissing() const;
    DeBruijnEdge *getSelfLoopingEdge() const;
    int getDeadEndCount() const;

    void setSequence(const QByteArray &newSeq) {m_sequence = Sequence(newSeq); m_length = m_sequence.size();}
    void setSequence(const Sequence &newSeq) {m_sequence = newSeq; m_length = m_sequence.size();}
    void setReverseComplement(DeBruijnNode * rc) {m_reverseComplement = rc;}
    void setGraphicsItemNode(GraphicsItemNode * gin) {m_graphicsItemNode = gin;}
    void setAsSpecial() {m_specialNode = true;}
    void setAsNotSpecial() {m_specialNode = false;}
    void setAsDrawn() {m_drawn = true;}
    void setAsNotDrawn() {m_drawn = false;}
    void resetNode();
    void addEdge(DeBruijnEdge * edge);
    void removeEdge(DeBruijnEdge * edge);
    void labelNeighbouringNodesAsDrawn(int nodeDistance);
    void setDepth(double newDepth) {m_depth = newDepth;}
    void setName(QString newName) {m_name = std::move(newName);}
    void addHiCEdge(HiCEdge * edge);
    void setComponentId(int componentId) {m_componentId = componentId;}
    int getComponentId() {return m_componentId;}
    bool isNodeUnion() const {return false;}

    QColor m_customColor;
    QString m_customLabel;
    int m_graphId;

    const void setCustomColor(QColor color) { m_customColor = color; }
    void setCustomLabel(QString label) { m_customLabel = label; }
private:
    QString m_name;
    Sequence m_sequence;
    DeBruijnNode * m_reverseComplement;
    adt::SmallPODVector<DeBruijnEdge *> m_edges;
    adt::SmallPODVector<HiCEdge *> m_hicEdges;

    GraphicsItemNode * m_graphicsItemNode;

    float m_depth;

    unsigned m_length : 30;
    bool m_specialNode : 1;
    bool m_drawn : 1;
    int m_componentId = 0;

    QByteArray getNodeNameForFasta(bool sign) const;
    QByteArray getUpstreamSequence(int upstreamSequenceLength) const;

    static std::vector<DeBruijnNode *> getNodesCommonToAllPaths(std::vector< std::vector <DeBruijnNode *> > * paths,
                                                                bool includeReverseComplements) ;
    bool doesPathLeadOnlyToNode(DeBruijnNode * node, bool includeReverseComplement);
};
