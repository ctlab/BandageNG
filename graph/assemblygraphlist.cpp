#include "assemblygraphlist.h"
#include "program/globals.h"
#include "program/settings.h"

#include <iostream>
#include <filesystem>

#include<QDebug>

AssemblyGraphList::AssemblyGraphList()
{
}

QColor AssemblyGraphList::getCustomColourForDisplay(const DeBruijnNode *node) const {
    if (node->m_customColor.isValid())
        return node->m_customColor;

    DeBruijnNode *rc = node->getReverseComplement();
    if (!g_settings->doubleMode && rc->m_customColor.isValid())
        return rc->m_customColor;
    return g_settings->defaultCustomNodeColour;
}

QStringList AssemblyGraphList::getCustomLabelForDisplay(const DeBruijnNode *node) const {
    QStringList customLabelLines;
    QString label = node->m_customLabel;
    if (!label.isEmpty()) {
        QStringList labelLines = label.split("\n");
        for (auto & labelLine : labelLines)
            customLabelLines << labelLine;
    }

    DeBruijnNode *rc = node->getReverseComplement();
    if (!g_settings->doubleMode && !rc->m_customLabel.isEmpty()) {
        QStringList labelLines2 = rc->m_customLabel.split("\n");
        for (auto & i : labelLines2)
            customLabelLines << i;
    }
    return customLabelLines;
}


static QStringList removeNullStringsFromList(const QStringList& in) {
    QStringList out;

    for (const auto& string : in) {
        if (string.length() > 0)
            out.push_back(string);
    }
    return out;
}

bool AssemblyGraphList::checkIfStringHasNodes(QString nodesString) {
    nodesString = nodesString.simplified();
    QStringList nodesList = nodesString.split(",");
    nodesList = removeNullStringsFromList(nodesList);
    return (nodesList.empty());
}

QString AssemblyGraphList::generateNodesNotFoundErrorMessage(std::vector<QString> nodesNotInGraph, bool exact) {
    QString errorMessage;
    if (exact)
        errorMessage += "The following nodes are not in the graph:\n";
    else
        errorMessage += "The following queries do not match any nodes in the graph:\n";

    for (size_t i = 0; i < nodesNotInGraph.size(); ++i) {
        errorMessage += nodesNotInGraph[i];
        if (i != nodesNotInGraph.size() - 1)
            errorMessage += ", ";
    }
    errorMessage += "\n";

    return errorMessage;
}

void AssemblyGraphList::recalculateAllNodeWidths(double averageNodeWidth,
                              double depthPower, double depthEffectOnWidth)
{
    for (auto assemblyGraph : m_graphMap.values())
        assemblyGraph->recalculateAllNodeWidths(averageNodeWidth,depthPower, depthEffectOnWidth);
}

void AssemblyGraphList::changeNodeDepth(const std::vector<DeBruijnNode *> &nodes,
                                    double newDepth)
{
    if (nodes.empty())
        return;

    for (auto node : nodes) {
        node->setDepth(newDepth);
        node->getReverseComplement()->setDepth(newDepth);
    }

    for (auto node : nodes) {

        //If this graph does not already have a depthTag, give it a depthTag of KC
        //so the depth info will be saved.
        if (m_graphMap[node->getGraphId()]->m_depthTag == "")
            m_graphMap[node->getGraphId()]->m_depthTag = "KC";
    }
}

double AssemblyGraphList::getMeanDepth(const std::vector<DeBruijnNode *> &nodes)
{
    if (nodes.empty())
        return 0.0;

    if (nodes.size() == 1)
        return nodes[0]->getDepth();

    int nodeCount = 0;
    long double depthSum = 0.0;
    long long totalLength = 0;

    for (auto node : nodes)
    {
        ++nodeCount;
        totalLength += node->getLength();
        depthSum += node->getLength() * node->getDepth();
    }

    //If the total length is zero, that means all nodes have a length of zero.
    //In this case, just return the average node depth.
    if (totalLength == 0)
    {
        long double depthSum = 0.0;
        for (auto & node : nodes)
            depthSum += node->getDepth();
        return depthSum / nodes.size();
    }

    return depthSum / totalLength;
}

bool AssemblyGraphList::loadGraphsFromDir(const QString& basePath) {

    if (basePath.isEmpty()) //User did hit cancel
        return false;

    QString dirname = basePath;

    clear();

    loadGraphsIter(dirname, basePath);
    return true;
}

void AssemblyGraphList::loadGraphsIter(QString fullDirName, const QString& basePath) {
    if (fullDirName.isEmpty()) //User did hit cancel
        return;
    for (const auto entry : std::filesystem::directory_iterator(fullDirName.toStdString())) {
        const std::filesystem::path path = entry.path();
        if(std::filesystem::is_directory(path)) {
            loadGraphsIter(QString::fromStdString(path), basePath);
        }
        if (std::filesystem::is_regular_file(path) && path.extension() != ".fasta") {
            QString qpath = QString::fromStdString(path);

            qInfo() << "loadGraphFromFile: " << qpath;
            int graphId = g_assemblyGraph->m_graphMap.size() + 1;
            g_assemblyGraph->m_graphMap[graphId] = new AssemblyGraph();
            g_assemblyGraph->m_graphMap[graphId] -> setGraphId(graphId);
            bool loadSuccess = g_assemblyGraph->m_graphMap[graphId]->loadGraphFromFile(qpath);

            g_assemblyGraph->m_graphMap[graphId]->setGraphName(qpath.right(qpath.size() - basePath.size()));
        }
    }
}
