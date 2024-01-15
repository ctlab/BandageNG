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
        if (m_graphMap.isEmpty()) {
            auto assemblyGraph = new AssemblyGraph();
            m_graphMap[1] = assemblyGraph;
        }
        return m_graphMap[1];
    }
    //AssemblyGraph* last() { return m_graphList.last(); }
    bool isEmpty() { return m_graphMap.isEmpty(); }
    //auto begin() { return m_graphList.begin(); }
    //auto begin() const { return m_graphList.begin(); }
    //auto end() { return m_graphList.end(); }
    //auto end() const { return m_graphList.end(); }
    //void append(AssemblyGraph* graph) { m_graphList.append(graph); }
    void clear() { m_graphMap.clear(); }
    size_t size() const { return m_graphMap.size(); }

    QColor getCustomColourForDisplay(const DeBruijnNode *node) const;
    QStringList getCustomLabelForDisplay(const DeBruijnNode *node) const;

    int getNodeCount(){
        int count = 0;
        for (auto graph : m_graphMap.values()) {
            count += graph->m_nodeCount;
        }
        return count;
    }
    int getEdgeCount() {
        int count = 0;
        for (auto graph : m_graphMap.values()) {
            count += graph->m_edgeCount;
        }
        return count;
    }
    int getPathCount() {
        int count = 0;
        for (auto graph : m_graphMap.values()) {
            count += graph->m_pathCount;
        }
        return count;
    }
    int getTotalLength() {
        int count = 0;
        for (auto graph : m_graphMap.values()) {
            count += graph->m_totalLength;
        }
        return count;
    }

    void determineGraphInfo() {
        for (auto graph : m_graphMap.values()) {
            graph -> determineGraphInfo();
        }
    }
    static bool checkIfStringHasNodes(QString nodesString);
    static QString generateNodesNotFoundErrorMessage(std::vector<QString> nodesNotInGraph,
                                              bool exact);

    double getMeanDepth(const std::vector<DeBruijnNode *> &nodes);

    void changeNodeDepth(const std::vector<DeBruijnNode *> &nodes, double newDepth);
    void recalculateAllNodeWidths(double averageNodeWidth, double depthPower, double depthEffectOnWidth);

    QMap<int, AssemblyGraph*> m_graphMap;

private:

};

#endif // ASSEMBLYGRAPHLIST_H
