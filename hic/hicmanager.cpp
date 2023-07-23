#include "hicmanager.h"

HiCManager::HiCManager()
{}

bool HiCManager::load(AssemblyGraph &graph, QString filename, QString* errormsg)
{
    findComponents(graph);
    QFile hiCMatrix(filename);
    if (!hiCMatrix.open(QIODevice::ReadOnly))
    {
        *errormsg = "Unable to read from specified file.";
        return false;
    }

    QTextStream in(&hiCMatrix);
    QApplication::processEvents();
    QString line = in.readLine();
    int maxWeight = 0;

    while ((line = in.readLine()) != "") {
        QStringList data = line.split(QRegularExpression("\t"));
        QString firstNodeName;
        QString secondNodeName;
        if(data.at(0).left(2) == "Id")
            firstNodeName = convertNodeName(data.at(0).right(data.at(0).length() - 2));
        else
            firstNodeName = convertNodeName(data.at(0));

        if(data.at(1).left(2) == "Id")
            secondNodeName = convertNodeName(data.at(1).right(data.at(1).length() - 2));
        else
            secondNodeName = convertNodeName(data.at(1));

        int weight = data.at(2).toInt();

        if (weight > 0 && firstNodeName != secondNodeName) {
            createDeBruijnEdge(graph, firstNodeName, secondNodeName, weight);
        }
        if (weight > maxWeight) {
            maxWeight = weight;
        }
    }
    m_maxWeight = maxWeight;

    for (const auto &entry : graph.m_hicGraphEdges) {
        HiCEdge *edge = entry.second;
        addEdgeIfNeeded(edge);
    }
    return true;
}

void HiCManager::createDeBruijnEdge(AssemblyGraph &graph, const QString& node1Name, const QString& node2Name, const int weight)
{
    QString node1Opposite = getOppositeNodeName(node1Name);
    QString node2Opposite = getOppositeNodeName(node2Name);

    //Quit if any of the nodes don't exist.
    auto node1 = graph.m_deBruijnGraphNodes.find(node1Name.toStdString());
    auto node2 = graph.m_deBruijnGraphNodes.find(node2Name.toStdString());
    auto negNode1 = graph.m_deBruijnGraphNodes.find(node1Opposite.toStdString());
    auto negNode2 = graph.m_deBruijnGraphNodes.find(node2Opposite.toStdString());
    if (node1 == graph.m_deBruijnGraphNodes.end() ||
        node2 == graph.m_deBruijnGraphNodes.end() ||
        negNode1 == graph.m_deBruijnGraphNodes.end() ||
        negNode2 == graph.m_deBruijnGraphNodes.end())
        return;

    //Quit if the edge already exists
    for (const auto *edge : (*node1)->edges()) {
        if (edge->getStartingNode() == *node1 &&
            edge->getEndingNode() == *node2)
            return;
    }

    //Usually, an edge has a different pair, but it is possible
    //for an edge to be its own pair.
    bool isOwnPair = (*node1 == *negNode2 && *node2 == *negNode1);

    auto * forwardEdge = new HiCEdge(*node1, *node2, weight);
    HiCEdge * backwardEdge;

    if (isOwnPair)
        backwardEdge = forwardEdge;
    else
        backwardEdge = new HiCEdge(*negNode2, *negNode1, weight);

    forwardEdge->setReverseComplement(backwardEdge);
    backwardEdge->setReverseComplement(forwardEdge);

    graph.m_hicGraphEdges.emplace(std::make_pair(forwardEdge->getStartingNode(), forwardEdge->getEndingNode()), forwardEdge);
    if (!isOwnPair)
        graph.m_hicGraphEdges.emplace(std::make_pair(backwardEdge->getStartingNode(), backwardEdge->getEndingNode()), backwardEdge);

    (*node1)->addHiCEdge(forwardEdge);
    (*node2)->addHiCEdge(forwardEdge);
    (*negNode1)->addHiCEdge(backwardEdge);
    (*negNode2)->addHiCEdge(backwardEdge);
}

void HiCManager::findComponents(AssemblyGraph& graph) {
    wasComponentsFound = true;
    int componentId = 1;
    for (const auto &entry: graph.m_deBruijnGraphNodes) {
        DeBruijnNode *node = entry;
        if (node->getComponentId() == 0) {
            std::vector<DeBruijnNode*> mergedNode;
            QPair<unsigned int, unsigned long> res = dfsComponent(node, componentId, &mergedNode);
            if (res.first != 0) {
                componentSize.append(res.second);
                averageSize.append(res.second / (unsigned long)res.first);
                componentId++;
            }
        }

    }
    componentNum = componentId - 1;
}

QPair<unsigned int, unsigned long> HiCManager::dfsComponent(DeBruijnNode * node, int componentId, std::vector<DeBruijnNode *>* mergedNode) {
    unsigned long size = 0;
    unsigned int contigCount = 0;
    if (node->getComponentId() == 0) {
        node->setComponentId(componentId);
        node->getReverseComplement()->setComponentId(componentId);

        if (node->getNameWithoutSign().endsWith("_start")) {
            addTargetComponentIfNeeded(componentId);
        }
        for (DeBruijnEdge* edge : node->getLeavingEdges()) {
            if (edge->getEndingNode()->getComponentId() == 0) {
                QPair<unsigned int, unsigned long> res = dfsComponent(edge->getEndingNode(), componentId, mergedNode);
                size += res.second;
                contigCount += res.first;
            }
        }
        for (DeBruijnEdge* edge : node->getEnteringEdges()) {
            if (edge->getStartingNode()->getComponentId() == 0) {
                QPair<unsigned int, unsigned long> res = dfsComponent(edge->getStartingNode(), componentId, mergedNode);
                size += res.second;
                contigCount += res.first;
            }
        }
        size += node->getLength();
        contigCount += 1;
    }
    return qMakePair(contigCount, size);
}

QString HiCManager::convertNodeName(QString number)
{
    if (number.at(0) == '-')
    {
        number.remove(0, 1);
        return number + "-";
    }
    else
        return number + "+";
}

QString HiCManager::getOppositeNodeName(QString nodeName)
{
    QChar lastChar = nodeName.at(nodeName.size() - 1);
    nodeName.chop(1);

    if (lastChar == '-')
        return nodeName + "+";
    else
        return nodeName + "-";
}

bool HiCManager::isDrawnWithNode(const HiCEdge* edge) {
    return isDrawnWithNode(edge, this->m_inclusionFilter);
}

bool HiCManager::isDrawnWithNode(const HiCEdge* edge, HiCInclusionFilter filterHiC) {
    bool drawEdge = edge->getStartingNode()->isDrawn() && edge->getEndingNode()->isDrawn();
    if (!drawEdge)
        return false;
    bool res = (edge->getWeight() >= m_minWeight &&
        edge->getStartingNode()->getLength() >= m_minLength &&
        edge->getEndingNode()->getLength() >= m_minLength &&
        (filterHiC == ALL ||
            filterHiC == ALL_BETWEEN_GRAPH_COMPONENTS && edge->getStartingNode()->getComponentId() != edge->getEndingNode()->getComponentId() ||
            filterHiC == ONE_BETWEEN_GRAPH_COMPONENT && contains(edge) ||
            filterHiC == ONE_FROM_TARGET_COMPONENT && contains(edge) &&
                (isTargetComponent(edge->getStartingNode()->getComponentId()) || isTargetComponent(edge->getEndingNode()->getComponentId()))));
    return res;

}

bool HiCManager::isDrawn(const HiCEdge* edge) {
    return (edge->getWeight() >= m_minWeight &&
            edge->getStartingNode()->getLength() >= m_minLength &&
            edge->getEndingNode()->getLength() >= m_minLength &&
            (m_inclusionFilter == ALL || edge->getStartingNode()->getComponentId() != edge->getEndingNode()->getComponentId()));
}

bool HiCManager::addEdgeIfNeeded(HiCEdge* edge) {
    QPair<int, int> key = getComponentKey(edge);
    int startingComponentId = edge->getStartingNode()->getComponentId();
    int endingComponentId = edge->getEndingNode()->getComponentId();
    if (startingComponentId != endingComponentId &&
        isBigComponent(startingComponentId) &&
        isBigComponent(endingComponentId)) {
        sumWeightBetweenComponent += edge->getWeight();
        countOfEdgesBetweenComponent += 1;
        if (!componentEdgeMap.contains(key)) {
            componentEdgeMap[key] = edge;
            return true;
        }
        else if (componentEdgeMap[key]->getWeight() < edge -> getWeight()) {
            componentEdgeMap[key] = edge;
            return true;
        }
    }
    return false;
}

bool HiCManager::contains(const HiCEdge* edge) {
    QPair<int, int> key = getComponentKey(edge);
    return componentEdgeMap.contains(key) &&
        componentEdgeMap[key] == edge;
}

void HiCManager::addTargetComponentIfNeeded(int id) {
    if (!targetComponents.contains(id)) {
        targetComponents.push_back(id);
    }
}

bool HiCManager::isTargetComponent(int componentId) {
    return targetComponents.contains(componentId);
}

bool HiCManager::isConnectedWithTargetComponent(int componentId) {
    if (isTargetComponent(componentId)) {
        return true;
    }
    for (int targetComponentId : targetComponents) {
        QPair<int, int> key = qMakePair(componentId, targetComponentId);
        QPair<int, int> reverseKey = qMakePair(targetComponentId, componentId);
        if ((componentEdgeMap.contains(key) /*&& componentEdgeMap[key]->getWeight() >= m_minWeight*/ ) ||
            (componentEdgeMap.contains(reverseKey) /*&& componentEdgeMap[reverseKey]->getWeight() >= m_minWeight*/)) {
            return true;
        }
    }
    return false;
}

bool HiCManager::isBigComponent(int componentId) {
    return componentSize[componentId - 1] >= 500;
}

QPair<int, int> HiCManager::getComponentKey(const HiCEdge* edge) {
    if (edge->getStartingNode()->isPositiveNode() && edge->getEndingNode()->isPositiveNode())
        return qMakePair(std::min(edge->getStartingNode()->getComponentId(), edge->getEndingNode()->getComponentId()),
            std::max(edge->getStartingNode()->getComponentId(), edge->getEndingNode()->getComponentId()));
    else
        return qMakePair(std::max(edge->getStartingNode()->getComponentId(), edge->getEndingNode()->getComponentId()),
            std::min(edge->getStartingNode()->getComponentId(), edge->getEndingNode()->getComponentId()));
}
