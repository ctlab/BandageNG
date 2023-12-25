#ifndef ASSEMBLYGRAPHLIST_H
#define ASSEMBLYGRAPHLIST_H

#include <QList>
#include "graph/assemblygraph.h"
#include "graph/debruijnnode.h"

class AssemblyGraphList
{
public:
    AssemblyGraphList();

    AssemblyGraph* first() {
        if (m_graphList.isEmpty()) {
            auto assemblyGraph = new AssemblyGraph();
            m_graphList.append(assemblyGraph);
        }
        return m_graphList.first();
    }
    AssemblyGraph* last() { return m_graphList.last(); }
    bool isEmpty() { return m_graphList.isEmpty(); }
    auto begin() { return m_graphList.begin(); }
    auto begin() const { return m_graphList.begin(); }
    auto end() { return m_graphList.end(); }
    auto end() const { return m_graphList.end(); }
    void append(AssemblyGraph* graph) { m_graphList.append(graph); }
    void clear() { m_graphList.clear(); }
    size_t size() const { return m_graphList.size(); }

    QColor getCustomColourForDisplay(const DeBruijnNode *node) const;
    QStringList getCustomLabelForDisplay(const DeBruijnNode *node) const;

    int getNodeCount(){
        int count = 0;
        for (auto graph : m_graphList) {
            count += graph->m_nodeCount;
        }
        return count;
    }
    int getEdgeCount() {
        int count = 0;
        for (auto graph : m_graphList) {
            count += graph->m_edgeCount;
        }
        return count;
    }
    int getPathCount() {
        int count = 0;
        for (auto graph : m_graphList) {
            count += graph->m_pathCount;
        }
        return count;
    }
    int getTotalLength() {
        int count = 0;
        for (auto graph : m_graphList) {
            count += graph->m_totalLength;
        }
        return count;
    }

    void determineGraphInfo() {
        for (auto graph : m_graphList) {
            graph -> determineGraphInfo();
        }
    }
    static bool checkIfStringHasNodes(QString nodesString);
    static QString generateNodesNotFoundErrorMessage(std::vector<QString> nodesNotInGraph,
                                              bool exact);

    double getMeanDepth(const std::vector<DeBruijnNode *> &nodes);

    void changeNodeDepth(const std::vector<DeBruijnNode *> &nodes, double newDepth);
    void recalculateAllNodeWidths(double averageNodeWidth, double depthPower, double depthEffectOnWidth);

    QList<AssemblyGraph*> m_graphList;

private:

};

#endif // ASSEMBLYGRAPHLIST_H
