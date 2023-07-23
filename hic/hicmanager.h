#ifndef HICMANAGER_H
#define HICMANAGER_H

#include "graph/assemblygraph.h"
#include "hic/hicedge.h"
#include "graph/debruijnnode.h"

#include <QFileInfo>
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QString>
#include <QRegularExpression>

enum HiCDrawingType {ALL_EDGES, ONE_EDGE, NO_EDGE};
enum HiCInclusionFilter {ALL, ALL_BETWEEN_GRAPH_COMPONENTS, ONE_BETWEEN_GRAPH_COMPONENT, ONE_FROM_TARGET_COMPONENT};

class HiCManager
{
public:
    HiCManager();
    void findComponents(AssemblyGraph &graph);
    bool load(AssemblyGraph &graph, QString filename, QString* errormsg);
    int getMaxWeight(){return m_maxWeight;}
    void setMinWeight(int minWeight){m_minWeight = minWeight;}
    void setMinLength(int minLen){m_minLength = minLen;}
    bool isDrawnWithNode(const HiCEdge* edge);
    bool isDrawnWithNode(const HiCEdge* edge, HiCInclusionFilter filterHiC);
    bool isDrawn(const HiCEdge* edge);
    void setHiCInclusionFilter(HiCInclusionFilter filter){m_inclusionFilter = filter;}
    HiCInclusionFilter getHiCInclusionFilter(){return m_inclusionFilter;}
private:
    int m_minWeight = 0;
    int m_minLength = 1;
    int m_maxWeight = 0;
    bool checkContigLength = true;
    unsigned long sumWeightBetweenComponent = 0;
    unsigned long countOfEdgesBetweenComponent = 0;

    HiCInclusionFilter m_inclusionFilter = ALL;
    int componentNum = 0;
    QMap<QPair<int, int>, HiCEdge*> componentEdgeMap;
    QVector<int> targetComponents;
    QVector<int> componentSize;
    QVector<int> averageSize;

    void createDeBruijnEdge(AssemblyGraph &graph, const QString& node1Name, const QString& node2Name, const int weight);
    QString convertNodeName(QString number);
    QString getOppositeNodeName(QString nodeName);
    bool addEdgeIfNeeded(HiCEdge* edge);
    bool contains(const HiCEdge* edge);
    void addTargetComponentIfNeeded(int id);
    bool isTargetComponent(int componentId);
    bool isConnectedWithTargetComponent(int componentId);
    bool isBigComponent(int componentId);
    int getAverageWeightBetweenComponent() {
        if (countOfEdgesBetweenComponent == 0) {
            return 0;
        }
        return sumWeightBetweenComponent / countOfEdgesBetweenComponent;
    }
    QPair<int, int> getComponentKey(const HiCEdge* edge);
    bool wasComponentsFound = false;
    QPair<unsigned int, unsigned long> dfsComponent(DeBruijnNode * node, int componentId, std::vector<DeBruijnNode *>* mergedNode);
};

#endif // HICMANAGER_H
